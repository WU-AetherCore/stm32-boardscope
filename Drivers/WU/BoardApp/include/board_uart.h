/** @file board_uart.h
 * @brief 六路串口的循环 DMA 接收、复制发送队列和监测接口。
 * port 使用 0..5，对应 UART1..6；数据长度始终显式传递，支持二进制。
 */
#ifndef BOARD_UART_H
#define BOARD_UART_H
#include "main.h"
#include <stdbool.h>
#define BOARD_UART_COUNT 6U
#define BOARD_HISTORY 256U
#define BOARD_UART_PACKET_SIZE 128U
#define BOARD_UART_QUEUE_DEPTH 8U
extern volatile uint32_t board_rx[6], board_tx[6], board_drop[6], board_uart_errors[6];
extern volatile uint32_t board_rx_drop[6], board_tx_failed[6];
extern bool board_uart_echo[6];
/* 初始化接收，在调度器启动前调用；之后由 Service 自动恢复故障接收。 */
void Board_UartStart(void);
/* 只在独立通信任务调用。处理命令、接收队列和发送队列，不阻塞等待 TX 完成。 */
void Board_UartService(void);
/* 复制数据进队列，成功仅表示入队；完成字节数由 TX 完成中断更新。 */
bool Board_Send(unsigned port, const uint8_t *data, uint16_t size);
/* ISR/测试入口：记录 RX 并复制到业务队列，不在中断中解析命令。 */
void Board_Rx(unsigned port, const uint8_t *data, uint16_t size);
void Board_UartRxEvent(UART_HandleTypeDef *uart, uint16_t position);
void Board_UartTxComplete(UART_HandleTypeDef *uart);
void Board_UartError(UART_HandleTypeDef *uart);
/* 原子截取最近的数据：offset 为距离最新数据的字节数，ASCII 控制字节显示为点。 */
void Board_History(unsigned port, bool tx, unsigned offset, bool hex, char *out, unsigned capacity);
uint16_t Board_UartPending(unsigned port);
void Board_UartClearHistory(unsigned port);
#endif
