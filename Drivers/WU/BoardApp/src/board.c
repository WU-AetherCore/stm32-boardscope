/** @file board.c
 * @brief 板级业务：Flash 查询、CAN 调度以及三个普通按键的独立手势识别。
 * UI 不读取普通按键，按键事件通过弱回调交给用户业务。
 */
#include "board.h"
#include "spi.h"
#include "can.h"
#include "../../wu_can/wu_can.h"
#include "cmsis_os.h"
#include <stdio.h>
#include <string.h>
/* 温湿度由传感器任务更新；UI 只读取，不触碰 DHT11 总线。 */
volatile uint32_t board_key_events[3][3];
volatile uint8_t board_dht_ok;
volatile uint32_t board_dht_last;
uint32_t board_flash_id, board_flash_bytes;
uint8_t board_flash_status, board_flash_data[16];
bool board_flash_ok;
volatile uint32_t board_can_rx;
volatile uint8_t board_debug_ok;
volatile int16_t board_debug_turn;

/* SPI Flash 的所有事务均有超时；当前版本只提供读取功能。 */
static bool flash_transfer(uint8_t *cmd, unsigned n, uint8_t *out, unsigned size)
{
    HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef s = HAL_SPI_Transmit(&hspi2, cmd, n, 20);
    if (s == HAL_OK)
        s = HAL_SPI_Receive(&hspi2, out, size, 20);
    HAL_GPIO_WritePin(SPI2_CS_GPIO_Port, SPI2_CS_Pin, GPIO_PIN_SET);
    return s == HAL_OK;
}
void Board_FlashRead(uint32_t addr)
{
    uint8_t cmd = 0x9F, id[3] = {0};
    board_flash_ok = flash_transfer(&cmd, 1, id, 3);
    board_flash_id = (id[0] << 16) | (id[1] << 8) | id[2];
    board_flash_ok = board_flash_ok && id[0] != 0 && id[0] != 255 && id[2] >= 16 && id[2] <= 24;
    board_flash_bytes = board_flash_ok ? (1UL << id[2]) : 0;
    if (!board_flash_ok)
        return;
    cmd = 5;
    board_flash_ok = flash_transfer(&cmd, 1, &board_flash_status, 1);
    uint8_t read[] = {3, addr >> 16, addr >> 8, addr};
    if (addr <= board_flash_bytes - 16)
        board_flash_ok = flash_transfer(read, 4, board_flash_data, 16) && board_flash_ok;
    else
        board_flash_ok = false; // 超出容量时不显示上一次读到的数据为本地址内容。
}
/* 板级调度只处理 CAN；串口的队列和 DMA 由独立模块维护。 */
void Board_Service(void)
{
    if (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) &&
        WU_CAN_R(&RxID, &RxLength, RxData) == HAL_OK)
    {
        board_can_rx++;
    }
}
/* 用户可在独立 .c 文件中实现同名函数，覆盖默认空业务。 */
__attribute__((weak)) void Board_OnKeyEvent(uint8_t key, uint8_t event)
{
    (void)key;
    (void)event;
}
/* 每 10ms 从按键任务调用。25ms 去抖、800ms 长按、300ms 双击窗口。
 * 时间差使用无符号减法，可跨 HAL 毫秒计数器回绕。
 */
void Board_KeyScan(uint32_t now)
{
    typedef struct
    {
        bool raw, down, long_sent, pending;
        uint32_t changed, pressed, released;
    } Key;
    static Key k[3];
    GPIO_TypeDef *gp[] = {WU_KEY0_GPIO_Port, WU_KEY1_GPIO_Port, WU_KEY2_GPIO_Port};
    uint16_t pins[] = {WU_KEY0_Pin, WU_KEY1_Pin, WU_KEY2_Pin};
    for (unsigned i = 0; i < 3; i++)
    {
        Key *s = &k[i];
        bool down = HAL_GPIO_ReadPin(gp[i], pins[i]) == GPIO_PIN_RESET;
        int event = -1;
        if (down != s->raw)
        {
            s->raw = down;
            s->changed = now;
        }
        if (now - s->changed >= 25 && s->down != down)
        {
            s->down = down;
            if (down)
            {
                s->pressed = now;
                s->long_sent = false;
            }
            else if (!s->long_sent)
            {
                if (s->pending && now - s->released <= 300)
                {
                    event = 2;
                    s->pending = false;
                }
                else
                {
                    s->pending = true;
                    s->released = now;
                }
            }
        }
        if (s->down && !s->long_sent && now - s->pressed >= 800)
        {
            s->long_sent = true;
            s->pending = false;
            event = 1;
        }
        if (!s->down && s->pending && now - s->released > 300)
        {
            s->pending = false;
            event = 0;
        }
        if (event >= 0)
        {
            board_key_events[i][event]++;
            Board_OnKeyEvent(i, event);
        }
    }
}
