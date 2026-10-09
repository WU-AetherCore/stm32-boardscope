/** @file board_telemetry.c
 * @brief BoardScope 状态上报。固定缓冲区、非阻塞队列，无动态内存分配。
 * 每秒截取一组快照，各 JSON 行共享序号；end 表示本组已全部入队。
 * 发送队列拥堵时保留当前行稍后重试，禁止阻塞 OLED 和传感器任务。
 * GPIO 是寄存器采样；继电器字段是软件命令值，不代表触点反馈。
 */
#include "board_telemetry.h"
#include "board.h"
#include "board_memory.h"
#include "can.h"
#include "../../wu_can/wu_can.h"
#include "FreeRTOS.h"
#include "task.h"
#include "cmsis_os.h"
#include "astra_port.h"
#include <stdio.h>
#include <string.h>
extern unsigned char buzzer_flag_dat1, cs_bj_flag_bit;
extern volatile uint32_t board_ui_heartbeat, board_ui_stack_free_words;
extern volatile uint32_t board_oled_status;
extern volatile uint32_t board_ui_status;
/* 大数组放在静态区，避免消耗通信任务栈。单个快照内数值不再改变。
 * 跨任务状态按字段采样，不宣称是硬件总线同时锁存的原子快照。 */
static uint32_t values[26][8];
static const char *const kinds[] = {
    "begin", "sensor",     "relay", "flash",    "memory", "sections", "heap", "ui",   "key",
    "key",   "key",        "can",   "can_data", "uart",   "uart",     "uart", "uart", "uart",
    "uart",  "uart_state", "gpio",  "gpio",     "gpio",   "gpio",     "gpio", "end"};
static uint32_t sequence, last_capture, overruns;
static unsigned row = 26;
static void capture(uint32_t now)
{
    BoardMemory m;
    Board_MemorySnapshot(&m);
    memset(values, 0, sizeof values);
    values[0][0] = 1; /* 协议版本，后续字段为运行毫秒、系统时钟、周期。 */
    values[0][1] = now;
    values[0][2] = SystemCoreClock;
    values[0][3] = 1000;
    values[1][0] = Temperature;
    values[1][1] = Humidity;
    values[1][2] = board_dht_ok;
    values[1][3] = board_dht_last ? now - board_dht_last : UINT32_MAX;
    values[1][4] = Temperature_max;
    values[1][5] = T_alarm;
    values[1][6] = (uint32_t)(Voltage * 1000.0f + 0.5f);
    values[1][7] = 0; /* 电压为预设，ADC 实测有效标志固定为 0。 */
    values[2][0] = relay1_flag_dat;
    values[2][1] = relay2_flag_dat;
    values[2][2] = relay3_flag_dat;
    values[2][3] = relay4_flag_dat;
    values[2][4] = relay5_flag_dat;
    values[2][5] = buzzer_flag_dat1;
    values[2][6] = !cs_bj_flag_bit;
    values[3][0] = board_flash_ok;
    values[3][1] = board_flash_id;
    values[3][2] = board_flash_bytes;
    values[3][3] = board_flash_status;
    values[4][0] = m.flash_used;
    values[4][1] = m.flash_total;
    values[4][2] = m.ram_used;
    values[4][3] = m.ram_total;
    values[4][4] = m.ccm_used;
    values[4][5] = m.ccm_total;
    values[5][0] = m.text;
    values[5][1] = m.data;
    values[5][2] = m.bss;
    values[5][3] = m.dec;
    values[6][0] = m.heap_total;
    values[6][1] = m.heap_free;
    values[6][2] = m.heap_min_free;
    values[6][3] = m.task_count;
    values[6][4] = uxTaskGetStackHighWaterMark(NULL) * sizeof(StackType_t);
    values[7][0] = board_oled_status;
    values[7][1] = board_ui_status;
    values[7][2] = board_ui_heartbeat;
    values[7][3] = board_ui_stack_free_words * 4;
    for (unsigned i = 0; i < 3; ++i)
    {
        values[8 + i][0] = i + 1;
        for (unsigned j = 0; j < 3; ++j)
            values[8 + i][j + 1] = board_key_events[i][j];
    }
    values[11][0] = board_can_rx;
    values[11][1] = RxID;
    values[11][2] = RxLength;
    values[11][3] = HAL_CAN_GetError(&hcan1);
    for (unsigned i = 0; i < 8; ++i)
        values[12][i] = RxData[i];
    for (unsigned i = 0; i < 6; ++i)
    {
        values[13 + i][0] = i + 1;
        values[13 + i][1] = board_rx[i];
        values[13 + i][2] = board_tx[i];
        values[13 + i][3] = board_drop[i];
        values[13 + i][4] = board_rx_drop[i];
        values[13 + i][5] = board_uart_errors[i];
        values[13 + i][6] = board_tx_failed[i];
        /* 每路使用两个位组：低 4 位 pending，bit4 echo。 */
        values[19][i] = Board_UartPending(i) | (board_uart_echo[i] ? 16U : 0U);
    }
    GPIO_TypeDef *gp[] = {GPIOA, GPIOB, GPIOC, GPIOD, GPIOE};
    for (unsigned i = 0; i < 5; ++i)
    {
        values[20 + i][0] = i;
        values[20 + i][1] = gp[i]->IDR;
        values[20 + i][2] = gp[i]->ODR;
    }
    values[25][0] = overruns;
    values[25][1] = 26;
    ++sequence;
    row = 0;
}
void Board_CommTask(void const *argument)
{
    (void)argument;
    static const uint8_t counts[] = {4, 8, 7, 4, 6, 4, 5, 4, 4, 4, 4, 4, 8,
                                     7, 7, 7, 7, 7, 7, 6, 3, 3, 3, 3, 3, 2};
    char line[BOARD_UART_PACKET_SIZE];
    for (;;)
    {
        Board_UartService();
        uint32_t now = HAL_GetTick();
        if (now - last_capture >= 1000U)
        {
            last_capture = now;
            if (row == 26)
                capture(now);
            else
                ++overruns; /* 阻塞时不覆盖尚未发完的快照。 */
        }
        if (row < 26 && Board_UartPending(0) < BOARD_UART_QUEUE_DEPTH - 2)
        {
            int n = snprintf(line, sizeof line, "{\"s\":%lu,\"t\":\"%s\",\"v\":[",
                             (unsigned long)sequence, kinds[row]);
            for (unsigned i = 0; i < counts[row] && n > 0 && n < (int)sizeof line; ++i)
                n += snprintf(line + n, sizeof line - (unsigned)n, "%s%lu", i ? "," : "",
                              (unsigned long)values[row][i]);
            if (n > 0 && n + 4 < (int)sizeof line)
            {
                memcpy(line + n, "]}\r\n", 4);
                if (Board_Send(0, (const uint8_t *)line, (uint16_t)(n + 4)))
                    ++row;
            }
            /* 任意 uint32 字段最长十位；最长 UART 行小于 128 字节。 */
        }
        osDelay(5U);
    }
}
