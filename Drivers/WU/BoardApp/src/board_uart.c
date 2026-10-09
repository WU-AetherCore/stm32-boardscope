/** @file board_uart.c
 * @brief 六路串口管理。ISR 只收集数据；业务解析及 DMA 启动在通信任务执行。
 * RX 使用循环 DMA，避免每包重新启动接收造成窗口；TX 缓冲在完成中断之前保持不变。
 * 所有队列固定分配，不使用 malloc。队列满时拒绝新包，并累计丢包计数。
 */
#include "board_uart.h"
#include "board.h"
#include <stdio.h>
#include <string.h>

volatile uint32_t board_rx[6], board_tx[6], board_drop[6], board_uart_errors[6];
volatile uint32_t board_rx_drop[6], board_tx_failed[6];
/* 回显默认关闭，避免外接设备之间形成回显循环；可在 UI 中逐路开启。 */
bool board_uart_echo[6];
static UART_HandleTypeDef *const ports[6] = {&huart1, &huart2, &huart3, &huart4, &huart5, &huart6};
static uint8_t *const rx_buffers[6] = {wu_r1_dat, wu_r2_dat, wu_r3_dat,
                                       wu_r4_dat, wu_r5_dat, wu_r6_dat};
typedef struct
{
    uint8_t data[BOARD_UART_PACKET_SIZE];
    uint16_t len;
} Packet;
typedef struct
{
    Packet packets[BOARD_UART_QUEUE_DEPTH];
    volatile uint8_t read, write;
} PacketQueue;
static PacketQueue tx_queue[6], rx_queue[6];
static Packet tx_active[6];
static volatile bool tx_busy[6], recover_rx[6];
static uint32_t tx_started_at[6];
static uint16_t rx_position[6];
static uint8_t history[6][2][BOARD_HISTORY];
static uint32_t history_head[6][2];

/* 保存并恢复 PRIMASK，兼容任务和中断调用，不会错误地开启原本被屏蔽的中断。 */
static uint32_t lock(void)
{
    uint32_t state = __get_PRIMASK();
    __disable_irq();
    return state;
}
static void unlock(uint32_t state)
{
    __set_PRIMASK(state);
}
static int port_index(UART_HandleTypeDef *uart)
{
    for (unsigned i = 0; i < BOARD_UART_COUNT; ++i)
        if (ports[i] == uart)
            return (int)i;
    return -1;
}
static bool push(PacketQueue *queue, const uint8_t *data, uint16_t size)
{
    uint32_t state = lock();
    uint8_t next = (queue->write + 1U) % BOARD_UART_QUEUE_DEPTH;
    if (next == queue->read)
    {
        unlock(state);
        return false;
    }
    memcpy(queue->packets[queue->write].data, data, size);
    queue->packets[queue->write].len = size;
    queue->write = next;
    unlock(state);
    return true;
}
static bool pop(PacketQueue *queue, Packet *packet)
{
    uint32_t state = lock();
    if (queue->read == queue->write)
    {
        unlock(state);
        return false;
    }
    *packet = queue->packets[queue->read];
    queue->read = (queue->read + 1U) % BOARD_UART_QUEUE_DEPTH;
    unlock(state);
    return true;
}
static void record(unsigned port, unsigned direction, const uint8_t *data, unsigned size)
{
    uint32_t state = lock();
    for (unsigned i = 0; i < size; ++i)
    {
        history[port][direction][history_head[port][direction]++ % BOARD_HISTORY] = data[i];
    }
    unlock(state);
}
bool Board_Send(unsigned port, const uint8_t *data, uint16_t size)
{
    if (port >= BOARD_UART_COUNT || data == NULL || size == 0 || size > BOARD_UART_PACKET_SIZE)
        return false;
    if (!push(&tx_queue[port], data, size))
    {
        board_drop[port]++;
        return false;
    }
    return true;
}
void Board_Rx(unsigned port, const uint8_t *data, uint16_t size)
{
    if (port >= BOARD_UART_COUNT || data == NULL || size == 0 || size > BOARD_UART_PACKET_SIZE)
        return;
    record(port, 0, data, size);
    board_rx[port] += size;
    if (!push(&rx_queue[port], data, size))
        board_rx_drop[port]++;
}
static bool start_receive(unsigned port)
{
    rx_position[port] = 0;
    /* 半满与满中断均保留：连续无 IDLE 的流量也会被及时搬运。 */
    return HAL_UARTEx_ReceiveToIdle_DMA(ports[port], rx_buffers[port], 100) == HAL_OK;
}
void Board_UartStart(void)
{
    for (unsigned port = 0; port < BOARD_UART_COUNT; ++port)
        recover_rx[port] = !start_receive(port);
}
void Board_UartRxEvent(UART_HandleTypeDef *uart, uint16_t position)
{
    int index = port_index(uart);
    if (index < 0 || position > 100)
        return;
    unsigned port = (unsigned)index;
    uint16_t previous = rx_position[port];
    /* HAL 回调 Size 表示 DMA 写入位置，不是这一回调收到的新字节数。
     * IDLE/HT/TC 可能报告相同位置；相同位置不重复统计。
     */
    if (position == previous)
        return;
    if (position > previous)
        Board_Rx(port, rx_buffers[port] + previous, position - previous);
    else
    {
        if (previous < 100)
            Board_Rx(port, rx_buffers[port] + previous, 100 - previous);
        if (position > 0)
            Board_Rx(port, rx_buffers[port], position);
    }
    rx_position[port] = position;
}
void Board_UartTxComplete(UART_HandleTypeDef *uart)
{
    int index = port_index(uart);
    if (index < 0 || !tx_busy[index])
        return;
    /* TX 统计只计入已发生完成回调的字节；并不表示对端已收到或认可。 */
    record((unsigned)index, 1, tx_active[index].data, tx_active[index].len);
    board_tx[index] += tx_active[index].len;
    tx_busy[index] = false;
}
void Board_UartError(UART_HandleTypeDef *uart)
{
    int index = port_index(uart);
    if (index >= 0)
    {
        board_uart_errors[index]++;
        recover_rx[index] = true;
    }
}
uint16_t Board_UartPending(unsigned port)
{
    if (port >= BOARD_UART_COUNT)
        return 0;
    uint32_t state = lock();
    uint16_t count = (tx_queue[port].write + BOARD_UART_QUEUE_DEPTH - tx_queue[port].read) %
                     BOARD_UART_QUEUE_DEPTH;
    if (tx_busy[port])
        count++;
    unlock(state);
    return count;
}
void Board_UartClearHistory(unsigned port)
{
    if (port >= BOARD_UART_COUNT)
        return;
    uint32_t state = lock();
    memset(history[port], 0, sizeof history[port]);
    history_head[port][0] = history_head[port][1] = 0;
    unlock(state);
    /* 历史清除不改变累计计数，避免混淆统计口径。 */
}
void Board_History(unsigned port, bool tx, unsigned offset, bool hex, char *out, unsigned capacity)
{
    if (out == NULL || capacity == 0)
        return;
    out[0] = 0;
    if (port >= BOARD_UART_COUNT)
        return;
    uint8_t copy[16];
    unsigned count = 0;
    uint32_t state = lock(), total = history_head[port][tx];
    uint32_t available = total > BOARD_HISTORY ? BOARD_HISTORY : total;
    unsigned width = hex ? 5 : 16;
    if (offset < available)
    {
        count = available - offset;
        if (count > width)
            count = width;
        for (unsigned i = 0; i < count; ++i)
            copy[i] = history[port][tx][(total - offset - count + i) % BOARD_HISTORY];
    }
    unlock(state);
    unsigned used = 0;
    for (unsigned i = 0; i < count; ++i)
    {
        if (hex)
        {
            if (used + 3 >= capacity)
                break;
            used += (unsigned)snprintf(out + used, capacity - used, "%02X ", copy[i]);
        }
        else
        {
            if (used + 1 >= capacity)
                break;
            out[used++] = (copy[i] >= 32 && copy[i] <= 126) ? (char)copy[i] : '.';
        }
    }
    out[used] = 0;
}
/* UART1 为调试命令口。按行组包支持跨 DMA 分片，以及一包包含多条命令。
 * 超长命令整行丢弃，禁止把截断后的内容误作为合法命令执行。
 */
static void command(const char *line)
{
    char out[128], extra;
    int relay, value, delta;
    if (!strcmp(line, "STATUS"))
    {
        snprintf(out, sizeof out, "V1.2 T=%u H=%u DHT=%u R=%u%u%u%u%u FLASH=%06lX\r\n", Temperature,
                 Humidity, board_dht_ok, relay1_flag_dat, relay2_flag_dat, relay3_flag_dat,
                 relay4_flag_dat, relay5_flag_dat, (unsigned long)board_flash_id);
    }
    else if (sscanf(line, "RELAY %d %d %c", &relay, &value, &extra) == 2 && relay >= 1 &&
             relay <= 5 && (value == 0 || value == 1))
    {
        unsigned char *flags[] = {&relay1_flag_dat, &relay2_flag_dat, &relay3_flag_dat,
                                  &relay4_flag_dat, &relay5_flag_dat};
        *flags[relay - 1] = (unsigned char)value;
        strcpy(out, "OK\r\n");
    }
    else if (!strcmp(line, "UI OK"))
    {
        board_debug_ok = 1;
        strcpy(out, "OK\r\n");
    }
    else if (sscanf(line, "UI TURN %d %c", &delta, &extra) == 1 && delta >= -30 && delta <= 30)
    {
        board_debug_turn = (int16_t)delta;
        strcpy(out, "OK\r\n");
    }
    else if (!strcmp(line, "HELP"))
        strcpy(out, "STATUS / RELAY 1..5 0|1 / UI OK / UI TURN -30..30\r\n");
    else
        strcpy(out, "ERR command\r\n");
    Board_Send(0, (const uint8_t *)out, (uint16_t)strlen(out));
}
static void parse_uart1(const Packet *packet)
{
    static char line[128];
    static unsigned used;
    static bool overflow;
    static uint32_t seen_drops;
    if (seen_drops != board_rx_drop[0])
    {
        /* 丢包后废弃未完成的一行，直到下一个分隔符恢复同步，防止拼接错误指令。 */
        seen_drops = board_rx_drop[0];
        used = 0;
        overflow = true;
    }
    for (unsigned i = 0; i < packet->len; ++i)
    {
        char c = (char)packet->data[i];
        if (c == '\n' || c == '\r')
        {
            if (overflow)
                Board_Send(0, (const uint8_t *)"ERR line too long\r\n", 19);
            else if (used)
            {
                line[used] = 0;
                command(line);
            }
            used = 0;
            overflow = false;
        }
        else if (c == 0)
            overflow = true;
        else if (used + 1 < sizeof line)
            line[used++] = c;
        else
            overflow = true;
    }
}
/* 保留 UART2 旧三字节协议，状态机允许 Fa 指令跨 DMA 分片。 */
static void parse_uart2(const Packet *packet)
{
    static unsigned state;
    static uint8_t operation;
    static uint32_t seen_drops;
    if (seen_drops != board_rx_drop[1])
    {
        seen_drops = board_rx_drop[1];
        state = 0;
    }
    for (unsigned i = 0; i < packet->len; ++i)
    {
        uint8_t c = packet->data[i];
        if (state == 0)
        {
            if (c == 'F')
                state = 1;
        }
        else if (state == 1)
        {
            operation = c;
            state = (c == 'a' || c == 'b') ? 2 : 0;
        }
        else
        {
            if (operation == 'a' && c >= '0' && c <= '9')
            {
                unsigned char *flags[] = {&relay1_flag_dat, &relay2_flag_dat, &relay3_flag_dat,
                                          &relay4_flag_dat, &relay5_flag_dat};
                unsigned v = c - '0';
                *flags[v / 2] = (unsigned char)(v % 2);
            }
            if (operation == 'b' && (c == 'A' || c == 'B'))
                led_sgd = (c == 'B');
            state = 0;
        }
    }
}
void Board_UartService(void)
{
    Packet packet;
    for (unsigned port = 0; port < BOARD_UART_COUNT; ++port)
    {
        /* 有界处理：每轮最多两包，连续串口流量不会长期阻塞 UI。 */
        for (unsigned n = 0; n < 2 && pop(&rx_queue[port], &packet); ++n)
        {
            if (port == 0)
                parse_uart1(&packet);
            if (port == 1)
                parse_uart2(&packet);
            if (board_uart_echo[port])
                Board_Send(port, packet.data, packet.len);
        }
        if (recover_rx[port])
        {
            recover_rx[port] = false;
            HAL_UART_AbortReceive(ports[port]);
            if (!start_receive(port))
                recover_rx[port] = true;
        }
        /* 超时兜底：正常 128 字节在 115200bps 下约 12ms，500ms 仍未完成视为异常。
         * 防止错误或丢失完成中断导致该串口队列永久停滞。
         */
        if (tx_busy[port] && HAL_GetTick() - tx_started_at[port] >= 500)
        {
            HAL_UART_AbortTransmit(ports[port]);
            tx_busy[port] = false;
            board_tx_failed[port]++;
        }
        /* 不覆盖正在 DMA 发送的 active 缓冲区。发送失败时明确记账。 */
        if (!tx_busy[port] && ports[port]->gState == HAL_UART_STATE_READY &&
            pop(&tx_queue[port], &tx_active[port]))
        {
            tx_busy[port] = true;
            tx_started_at[port] = HAL_GetTick();
            if (HAL_UART_Transmit_DMA(ports[port], tx_active[port].data, tx_active[port].len) !=
                HAL_OK)
            {
                tx_busy[port] = false;
                board_tx_failed[port]++;
            }
        }
    }
}
