/** @file kk_ui_port.c
 * @brief OLED 独占任务：读取 TIM3 编码器与 PC2 按压、刷新 UI。
 * KEY0/1/2 不参与 UI；所有页面操作集中在此任务，避免多任务竞争显示缓冲区。
 * 任务每 5ms 让出 CPU；UI 本身按需绘制，数据文本每 250ms 刷新。
 */
#include "main.h"
#include "cmsis_os.h"
#include "tim.h"
#include "kk_oled.h"
#include "kk_ui.h"
#include "kk_ui_app.h"
#include "board.h"
/* 串口调试输入由 UI 任务统一派发，避免回调中直接调用 UI。 */
extern void KK_UI_DispatchInput(KK_UI_InputEvent event, uint32_t now);

/* 兼容旧编码器辅助函数；不要在其他任务修改 TIM3 的计数器。 */
unsigned int count;
/* 调试器和内存状态页使用，不产生额外串口输出。 */
volatile uint32_t kk_ui_heartbeat;
volatile uint32_t kk_ui_stack_free_words;
volatile OLED_Status kk_ui_oled_status = OLED_ERROR;
volatile KK_UI_Status kk_ui_status = KK_UI_NOT_INITIALIZED;

void OLED_WUStartTask04(void const *argument)
{
    (void)argument;
    while ((kk_ui_oled_status = OLED_Init()) != OLED_OK)
        osDelay(500U);
    KK_UI_AppReset(HAL_GetTick());
    kk_ui_status = KK_UI_Init(KK_UI_AppGet());
    uint16_t encoder_last = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
    int32_t encoder_remainder = 0;
    for (;;)
    {
        KK_UI_Input input = {0};
        if (HAL_GPIO_ReadPin(WU_KEY3_CODE_GPIO_Port, WU_KEY3_CODE_Pin) == GPIO_PIN_RESET)
            input.keys |= KK_UI_KEY_OK;
        Board_Service();
        uint16_t current = (uint16_t)__HAL_TIM_GET_COUNTER(&htim3);
        encoder_remainder += (int16_t)(uint16_t)(current - encoder_last);
        encoder_last = current;
        /* TIM3 TI1 模式：当前硬件每档两个计数；余数保留，避免丢失半档。 */
        input.encoder_delta = (int16_t)(encoder_remainder / 2);
        encoder_remainder %= 2;

        uint32_t now = HAL_GetTick();
        KK_UI_AppUpdate(now);
        kk_ui_status = KK_UI_Update(now, input);
        if (board_debug_turn)
        {
            int16_t turn = board_debug_turn;
            board_debug_turn = 0;
            KK_UI_InputEvent event = {turn < 0 ? KK_UI_INPUT_UP : KK_UI_INPUT_DOWN,
                                      KK_UI_INPUT_ENCODER, (uint16_t)(turn < 0 ? -turn : turn)};
            KK_UI_DispatchInput(event, now);
        }
        if (board_debug_ok)
        {
            board_debug_ok = 0;
            KK_UI_InputEvent event = {KK_UI_INPUT_OK, KK_UI_INPUT_PRESS, 1};
            KK_UI_DispatchInput(event, now);
        }
        KK_UI_AppProcessEvents();
        kk_ui_oled_status = OLED_GetLastStatus();
        ++kk_ui_heartbeat;
        kk_ui_stack_free_words = uxTaskGetStackHighWaterMark(NULL);
        osDelay(5U);
    }
}
