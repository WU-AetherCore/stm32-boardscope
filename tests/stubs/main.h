#ifndef MOCK_MAIN_H
#define MOCK_MAIN_H
#include <stdint.h>
#define HAL_UART_STATE_READY 0
#define HAL_UART_STATE_BUSY_TX 1
#define HAL_OK 0
#define HAL_ERROR 1
typedef struct { unsigned gState; } UART_HandleTypeDef;
extern UART_HandleTypeDef huart1,huart2,huart3,huart4,huart5,huart6;
extern uint8_t wu_r1_dat[100],wu_r2_dat[100],wu_r3_dat[100],wu_r4_dat[100],wu_r5_dat[100],wu_r6_dat[100];
extern unsigned char Temperature,Humidity,relay1_flag_dat,relay2_flag_dat,relay3_flag_dat,relay4_flag_dat,relay5_flag_dat,led_sgd;
unsigned __get_PRIMASK(void);void __disable_irq(void);void __set_PRIMASK(unsigned);
uint32_t HAL_GetTick(void);
int HAL_UARTEx_ReceiveToIdle_DMA(UART_HandleTypeDef*,uint8_t*,unsigned);
int HAL_UART_Transmit_DMA(UART_HandleTypeDef*,uint8_t*,unsigned);
int HAL_UART_AbortReceive(UART_HandleTypeDef*);
int HAL_UART_AbortTransmit(UART_HandleTypeDef*);
#endif
