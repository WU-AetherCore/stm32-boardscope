/*
 * wu_can.h
 *
 *  Created on: Feb 15, 2025
 *      Author: 17076
 */

#ifndef WU_WU_CAN_WU_CAN_H_
#define WU_WU_CAN_WU_CAN_H_


#include "main.h"


extern CAN_TxHeaderTypeDef TxHeader;/* 发送参数句柄 */
extern CAN_RxHeaderTypeDef RxHeader;/* 接收参数句柄 */
extern CAN_FilterTypeDef sFilterConfig;


extern uint32_t TxID;
extern uint8_t TxLength;
extern uint8_t TxData[8];

extern uint32_t RxID;
extern uint8_t RxLength;
extern uint8_t RxData[8];


void WU_CAN1_Init(void);

HAL_StatusTypeDef WU_CAN_T(uint32_t ID, uint8_t Length, uint8_t *Data);
uint8_t WU_CAN_ReceiveFlag(void);
HAL_StatusTypeDef WU_CAN_R(uint32_t *ID, uint8_t *Length, uint8_t *Data);
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan);
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan);




#endif /* WU_WU_CAN_WU_CAN_H_ */
