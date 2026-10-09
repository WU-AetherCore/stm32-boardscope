/** @file astra_port.h
 * @brief Astra OLED/任务适配的 C 接口。0 表示正常，1 表示屏幕通信错误。
 */
#ifndef ASTRA_PORT_H
#define ASTRA_PORT_H
#include <stdint.h>
#ifdef __cplusplus
extern "C"
{
#endif
    int Astra_DisplayInit(void);
    int Astra_DisplayWrite(const uint8_t *buffer);
    int Astra_DisplayContrast(uint8_t value);
    extern volatile uint32_t board_ui_heartbeat, board_ui_stack_free_words;
    extern volatile uint32_t board_oled_status, board_ui_status;
    extern volatile uint32_t astra_page, astra_selection, astra_alloc_failures;
#ifdef __cplusplus
}
#endif
#endif
