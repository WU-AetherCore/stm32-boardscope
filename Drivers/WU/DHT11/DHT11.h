#ifndef __DHT11_H
#define __DHT11_H

#include "main.h"

#define IO_DHT11     DHT11_Pin
#define GPIO_DHT11   DHT11_GPIO_Port

#define DHT11_DQ_High HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, 1)
#define DHT11_DQ_Low  HAL_GPIO_WritePin(DHT11_GPIO_Port, DHT11_Pin, 0)

void DHT11_IO_OUT(void);
void DHT11_IO_IN(void);
void DHT11_Init(void);
u8   DHT11_Read_Data(u8 *temp,u8 *humi);
u8   DHT11_Read_Byte(void);
u8   DHT11_Read_Bit(void);
u8   DHT11_Check(void);
void DHT11_Rst(void);

void DHT11_Init(void);

#endif

