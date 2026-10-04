/** @file board.h
 * @brief UI 与板级业务之间的兼容接口。串口实现参见 board_uart.h。
 */
#ifndef BOARD_H
#define BOARD_H
#include "main.h"
#include <stdbool.h>
#include "board_uart.h"
extern volatile uint32_t board_key_events[3][3];
extern volatile uint8_t board_dht_ok;
extern volatile uint32_t board_dht_last;
extern uint32_t board_flash_id, board_flash_bytes;
extern uint8_t board_flash_status;
extern bool board_flash_ok;
extern uint8_t board_flash_data[16];
extern volatile uint32_t board_can_rx;
extern volatile uint8_t board_debug_ok;
extern volatile int16_t board_debug_turn;
void Board_Service(void);
void Board_FlashRead(uint32_t address);
void Board_KeyScan(uint32_t now);
/* key=0/1/2 对应三个普通按键，event=0/1/2 对应短按/长按/双击。
 * 用户在独立业务文件中覆盖弱实现；回调在按键任务执行，不应长时间阻塞。 */
void Board_OnKeyEvent(uint8_t key, uint8_t event);
#endif
