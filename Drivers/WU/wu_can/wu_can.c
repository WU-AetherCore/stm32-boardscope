/*
 * wu_can.c
 *
 *  Created on: Feb 15, 2025
 *      Author: 17076
 */

#include "wu_can.h"
#include "can.h"

CAN_TxHeaderTypeDef TxHeader;/* 发送参数句柄 */
CAN_RxHeaderTypeDef RxHeader;/* 接收参数句柄 */
CAN_FilterTypeDef sFilterConfig;





uint32_t TxID = 0x333;
uint8_t TxLength = 8;
uint8_t TxData[8] = {0, 0, 0, 0, 0, 0, 0, 0};

uint32_t RxID;
uint8_t RxLength;
uint8_t RxData[8];


/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

/* CAN 正常模式初始化, 正常模式, 波特率 500Kbps */
//can_set_mode(CAN_MODE_NORMAL);

/* CAN 回环模式初始化, 回环模式, 波特率 500Kbps */
//can_set_mode(CAN_MODE_LOOPBACK);

/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/

void CAN_Filter_Config(void)
{
	/*配置CAN过滤器*/
	sFilterConfig.FilterBank = 0;                         	/* 过滤器0 */
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;     	/* 标识符屏蔽位模式 */
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;    	/* 长度32位位宽*/
	sFilterConfig.FilterIdHigh = 0x0000;                  	/* 32位ID */
	sFilterConfig.FilterIdLow = 0x0000;
	sFilterConfig.FilterMaskIdHigh = 0x0000;              	/* 32位MASK */
	sFilterConfig.FilterMaskIdLow = 0x0000;
	sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;	/* 过滤器0关联到FIFO0 */
	sFilterConfig.FilterActivation = CAN_FILTER_ENABLE; 	/* 激活滤波器0 */
	sFilterConfig.SlaveStartFilterBank = 14;

	/* 过滤器配置 */
	if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
	{

	}

	/* 启动CAN外围设备 */
	if (HAL_CAN_Start(&hcan1) != HAL_OK)
	{

	}
}


/**
 * @brief       CAN 模式设置
 * @note      	模式选择：环回模式、普通模式
 * @param       mode     : CAN_MODE_NORMAL,  正常模式;
 *                         CAN_MODE_LOOPBACK,回环模式;
 * @retval      无
 */
void can_set_mode(uint32_t mode)
{
	hcan1.Instance = CAN1;
	hcan1.Init.Prescaler = 4;
	hcan1.Init.Mode = mode;
	hcan1.Init.SyncJumpWidth = CAN_SJW_1TQ;
	hcan1.Init.TimeSeg1 = CAN_BS1_9TQ;
	hcan1.Init.TimeSeg2 = CAN_BS2_8TQ;
	hcan1.Init.TimeTriggeredMode = DISABLE;
	hcan1.Init.AutoBusOff = DISABLE;
	hcan1.Init.AutoWakeUp = DISABLE;
	hcan1.Init.AutoRetransmission = ENABLE;
	hcan1.Init.ReceiveFifoLocked = DISABLE;
	hcan1.Init.TransmitFifoPriority = DISABLE;
	if (HAL_CAN_Init(&hcan1) != HAL_OK)
	{
		Error_Handler();
	}
	/*配置CAN过滤器*/
	sFilterConfig.FilterBank = 0;                         	/* 过滤器0 */
	sFilterConfig.FilterMode = CAN_FILTERMODE_IDMASK;     	/* 标识符屏蔽位模式 */
	sFilterConfig.FilterScale = CAN_FILTERSCALE_32BIT;    	/* 长度32位位宽*/
	sFilterConfig.FilterIdHigh = 0x0000;                  	/* 32位ID */
	sFilterConfig.FilterIdLow = 0x0000;
	sFilterConfig.FilterMaskIdHigh = 0x0000;              	/* 32位MASK */
	sFilterConfig.FilterMaskIdLow = 0x0000;
	sFilterConfig.FilterFIFOAssignment = CAN_FILTER_FIFO0;	/* 过滤器0关联到FIFO0 */
	sFilterConfig.FilterActivation = CAN_FILTER_ENABLE; 		/* 激活滤波器0 */
	sFilterConfig.SlaveStartFilterBank = 14;

	/* 过滤器配置 */
	if (HAL_CAN_ConfigFilter(&hcan1, &sFilterConfig) != HAL_OK)
	{

	}

	/* 启动CAN外围设备 */
	if (HAL_CAN_Start(&hcan1) != HAL_OK)
	{

	}
}

/**
 * @brief       CAN 发送一组数据
 *   @note      发送格式固定为: 标准ID, 数据帧
 * @param       id      : 标准ID(11位)
 * @retval      发送状态 0, 成功; 1, 失败;
 */
uint8_t can_send_msg(uint32_t id, uint8_t *msg, uint8_t len)
{
  uint32_t TxMailbox = CAN_TX_MAILBOX0;

  TxHeader.StdId = id;         /* 标准标识符 */
  TxHeader.ExtId = id;         /* 扩展标识符(29位) 标准标识符情况下，该成员无效*/
  TxHeader.IDE = CAN_ID_STD;   /* 使用标准标识符 */
  TxHeader.RTR = CAN_RTR_DATA; /* 数据帧 */
  TxHeader.DLC = len;

  if (HAL_CAN_AddTxMessage(&hcan1, &TxHeader, msg, &TxMailbox) != HAL_OK) /* 发送消息 */
  {
    return 1;
  }

  while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) != 3); /* 等待发送完成,所有邮箱(有三个邮箱)为空 */

  return 0;
}

/**
 * @brief       CAN 接收数据查询
 *   @note      接收数据格式固定为: 标准ID, 数据帧
 * @param       id      : 要查询的 标准ID(11位)
 * @param       buf     : 数据缓存区
 * @retval      接收结果
 *   @arg       0   , 无数据被接收到;
 *   @arg       其他, 接收的数据长度
 */
uint8_t can_receive_msg(uint32_t id, uint8_t *buf)
{
  if (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) == 0)     /* 没有接收到数据 */
  {
    return 0;
  }

  if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &RxHeader, buf) != HAL_OK)  /* 读取数据 */
  {
    return 0;
  }

  if (RxHeader.StdId!= id || RxHeader.IDE != CAN_ID_STD || RxHeader.RTR != CAN_RTR_DATA)       /* 接收到的ID不对 / 不是标准帧 / 不是数据帧 */
  {
    return 0;
  }

  return RxHeader.DLC;
}

/*--------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/
void WU_CAN1_Init(void)
{
	// 初始化CAN1
	MX_CAN1_Init();

	// 配置CAN过滤器
	CAN_Filter_Config();

	//正常模式
	can_set_mode(CAN_MODE_NORMAL);

}

/**
 * @brief CAN数据发送函数
 * @param ID: 11位标准ID
 * @param Length: 数据长度(0-8)
 * @param Data: 发送数据指针
 * @retval HAL_StatusTypeDef 发送状态
 */
HAL_StatusTypeDef WU_CAN_T(uint32_t ID, uint8_t Length, uint8_t *Data)
{
	uint32_t TxMailbox;

	// 初始化发送报文头
	TxHeader.StdId = ID;
	TxHeader.ExtId = ID; // 在标准标识符模式下，扩展标识符不需要设置
	TxHeader.IDE = CAN_ID_STD; // 使用标准标识符
	TxHeader.RTR = CAN_RTR_DATA; // 数据帧
	TxHeader.DLC = Length; // 数据长度
	TxHeader.TransmitGlobalTime = DISABLE; // 不使用时间触发模式

	/* 复制数据 */
	for (uint8_t i = 0; i < Length; i++) {
		TxData[i] = Data[i];
	}

	/* 发送数据 */
	if (HAL_CAN_AddTxMessage(&hcan1, &TxHeader, TxData, &TxMailbox) != HAL_OK) {
		return HAL_ERROR;
	}

	/* 等待发送完成 */
	uint32_t tickstart = HAL_GetTick();
	while (HAL_CAN_GetTxMailboxesFreeLevel(&hcan1) != 3) {
		if ((HAL_GetTick() - tickstart) > 100) {
			return HAL_TIMEOUT;
		}
	}

	return HAL_OK;

}

/**
 * @brief 检查接收标志
 * @retval uint8_t 1-有数据,0-无数据
 */
uint8_t WU_CAN_ReceiveFlag(void)
{
	return (HAL_CAN_GetRxFifoFillLevel(&hcan1, CAN_RX_FIFO0) > 0) ? 1 : 0;
}

/**
 * @brief CAN数据接收函数
 * @param ID: 接收到的ID指针
 * @param Length: 接收数据长度指针
 * @param Data: 接收数据缓冲区
 * @retval HAL_StatusTypeDef 接收状态
 */
HAL_StatusTypeDef WU_CAN_R(uint32_t *ID, uint8_t *Length, uint8_t *Data)
{

	if (HAL_CAN_GetRxMessage(&hcan1, CAN_RX_FIFO0, &RxHeader, RxData) != HAL_OK) {
		return HAL_ERROR;
	}

	/* 解析ID */
	if (RxHeader.IDE == CAN_ID_STD) {
		*ID = RxHeader.StdId;
	} else {
		*ID = RxHeader.ExtId;
	}

	/* 处理数据 */
	if (RxHeader.RTR == CAN_RTR_DATA) {
		*Length = RxHeader.DLC;
		for (uint8_t i = 0; i < *Length; i++) {
			Data[i] = RxData[i];
		}
	} else {
		*Length = 0; // 远程帧处理
	}

	return HAL_OK;

}

/**
 * @brief CAN接收中断回调函数
 * @param hcan: CAN句柄
 */
void HAL_CAN_RxFifo0MsgPendingCallback(CAN_HandleTypeDef *hcan)
{
	// 在这里处理接收中断
	// 可以设置标志位或直接处理数据
}

/**
 * @brief CAN错误回调函数
 * @param hcan: CAN句柄
 */
void HAL_CAN_ErrorCallback(CAN_HandleTypeDef *hcan)
{
	// 处理CAN错误
}


/*----------------------------------------------------------------------------------------------------------------------------------------------------------------------------*/




