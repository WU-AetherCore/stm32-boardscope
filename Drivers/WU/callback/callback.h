/*
 * callback.h
 *
 *  Created on: Apr 5, 2024
 *      Author: wu
 */

#ifndef WU_CALLBACK_CALLBACK_H_
#define WU_CALLBACK_CALLBACK_H_


#include "main.h"

#include "../../WU/led/led.h"


extern unsigned char Seria3_Rx_String[];
extern uint8_t Seria3_TxPacket[];
extern uint8_t Seria3_RxPacket[];




void WU_USART3_TX(uint8_t wu_x);


#endif /* WU_CALLBACK_CALLBACK_H_ */
