/** @file callback.c
 * @brief HAL 串口回调适配。中断只采集数据和故障标志，不执行 UI 或业务命令。
 */
#include "callback.h"
#include "board_uart.h"
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *uart, uint16_t position)
{
    Board_UartRxEvent(uart, position);
}
void HAL_UART_TxCpltCallback(UART_HandleTypeDef *uart)
{
    Board_UartTxComplete(uart);
}
void HAL_UART_ErrorCallback(UART_HandleTypeDef *uart)
{
    Board_UartError(uart);
}
