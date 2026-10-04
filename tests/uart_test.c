/* 桌面测试真实 board_uart.c，模拟 HAL；不以测试代码替代生产状态机。 */
#include "board.h"
#include <assert.h>
#include <stdio.h>
#include <string.h>
UART_HandleTypeDef huart1, huart2, huart3, huart4, huart5, huart6;
uint8_t wu_r1_dat[100], wu_r2_dat[100], wu_r3_dat[100], wu_r4_dat[100], wu_r5_dat[100],
    wu_r6_dat[100];
unsigned char Temperature = 24, Humidity = 65, relay1_flag_dat, relay2_flag_dat, relay3_flag_dat,
              relay4_flag_dat, relay5_flag_dat, led_sgd;
volatile uint8_t board_dht_ok = 1, board_debug_ok;
volatile int16_t board_debug_turn;
uint32_t board_flash_id = 0xEF4018;
static uint32_t tick;
static unsigned irq_state, start_count;
static uint8_t *dma_pointer;
static unsigned dma_length;
static char transmitted[1024];
unsigned __get_PRIMASK(void)
{
    return irq_state;
}
void __disable_irq(void)
{
    irq_state = 1;
}
void __set_PRIMASK(unsigned state)
{
    irq_state = state;
}
uint32_t HAL_GetTick(void)
{
    return tick;
}
int HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef *h, uint8_t *b, unsigned n)
{
    (void)h;
    (void)b;
    assert(n == 100);
    start_count++;
    return HAL_OK;
}
int HAL_UART_Transmit_DMA(UART_HandleTypeDef *h, uint8_t *b, unsigned n)
{
    h->gState = HAL_UART_STATE_BUSY_TX;
    dma_pointer = b;
    dma_length = n;
    return HAL_OK;
}
int HAL_UART_AbortReceive(UART_HandleTypeDef *h)
{
    (void)h;
    return HAL_OK;
}
int HAL_UART_AbortTransmit(UART_HandleTypeDef *h)
{
    h->gState = HAL_UART_STATE_READY;
    return HAL_OK;
}
static void complete(UART_HandleTypeDef *h)
{
    assert(dma_length < sizeof transmitted);
    memcpy(transmitted, dma_pointer, dma_length);
    transmitted[dma_length] = 0;
    h->gState = HAL_UART_STATE_READY;
    Board_UartTxComplete(h);
}
int main(void)
{
    Board_UartStart();
    assert(start_count == 6);
    uint8_t caller[] = {1, 2, 3};
    assert(Board_Send(0, caller, 3));
    caller[0] = 99;
    Board_UartService();
    assert(dma_pointer[0] == 1);
    assert(board_tx[0] == 0);
    complete(&huart1);
    assert(board_tx[0] == 3);
    assert(Board_UartPending(0) == 0);
    assert(!Board_Send(6, caller, 3));
    assert(!Board_Send(0, NULL, 3));
    assert(!Board_Send(0, caller, 129));
    for (unsigned i = 0; i < 100; i++)
        wu_r3_dat[i] = (uint8_t)i;
    Board_UartRxEvent(&huart3, 50);
    assert(board_rx[2] == 50);
    Board_UartRxEvent(&huart3, 50);
    assert(board_rx[2] == 50);
    Board_UartRxEvent(&huart3, 100);
    assert(board_rx[2] == 100);
    Board_UartRxEvent(&huart3, 20);
    assert(board_rx[2] == 120);
    Board_UartService();
    Board_UartService();
    char text[32];
    Board_History(2, false, 0, true, text, sizeof text);
    assert(!strcmp(text, "0F 10 11 12 13 "));
    Board_History(99, false, 0, true, text, sizeof text);
    assert(text[0] == 0);
    Board_History(2, false, 0, true, text, 1);
    assert(text[0] == 0);
    Board_Rx(0, (const uint8_t *)"STA", 3);
    Board_UartService();
    Board_Rx(0, (const uint8_t *)"TUS\r\n", 5);
    Board_UartService();
    complete(&huart1);
    assert(strstr(transmitted, "T=24 H=65"));
    Board_Rx(0, (const uint8_t *)"RELAY 1 1 tail\n", 15);
    Board_UartService();
    complete(&huart1);
    assert(!relay1_flag_dat);
    assert(strstr(transmitted, "ERR"));
    Board_Rx(1, (const uint8_t *)"F", 1);
    Board_UartService();
    Board_Rx(1, (const uint8_t *)"a1", 2);
    Board_UartService();
    assert(relay1_flag_dat == 1);
    for (unsigned n = 0; n < 7; n++)
        assert(Board_Send(4, caller, 3));
    assert(!Board_Send(4, caller, 3));
    assert(board_drop[4] == 1);
    for (unsigned n = 0; n < 7; n++)
    {
        Board_UartService();
        complete(&huart5);
    }
    assert(Board_UartPending(4) == 0);
    assert(Board_Send(0, caller, 3));
    Board_UartService();
    tick = 501;
    Board_UartService();
    assert(board_tx_failed[0] == 1);
    assert(!Board_UartPending(0));
    Board_UartError(&huart3);
    Board_UartService();
    assert(start_count == 7);
    assert(board_uart_errors[2] == 1);
    Board_UartClearHistory(2);
    Board_History(2, false, 0, true, text, sizeof text);
    assert(text[0] == 0);
    assert(board_rx[2] == 120);
    puts("PASS: TX copy/completion, queue limits, DMA wrap/duplicates, split commands, strict "
         "syntax, UART2 compatibility, timeout/recovery, history bounds");
    return 0;
}
