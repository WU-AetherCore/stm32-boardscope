/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
  ******************************************************************************
  * @attention
  *
  * Copyright (c) 2024 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32f4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "dma.h"
#include "iwdg.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"



/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */


extern unsigned char Key_Down,Key_Up,Key_Val,Key_Old;
extern unsigned char T_10ms;
extern unsigned char Key_Flag_Bit;

extern unsigned char led_sgd;
extern unsigned char buz_sgd;
extern unsigned char Led_Flag_Bit;
extern unsigned int T_100ms;

extern unsigned char at_Flag_Bit;
extern unsigned int T_800ms;

extern unsigned int T_1500ms,T_1500ms_flag_dat;

extern unsigned char key_dat_1;//双击
extern unsigned char key_dat_2;//长按
extern unsigned char key_dat_3;//短按

extern unsigned int Key_Cnt_Flag;
extern unsigned char key_flag;


extern unsigned char oled_xs_dat;

extern unsigned char t_dat;

extern unsigned int count;


typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;

extern unsigned int T_5000ms;
extern unsigned char T_5000ms_flag_dat;


extern unsigned char relay1_flag_dat,relay2_flag_dat,relay3_flag_dat,relay4_flag_dat,relay5_flag_dat;

/*-------------------MCU数据上传------------------*/

extern unsigned char T_alarm; // 温度报警标志
extern unsigned char Temperature_max;//上限温度
extern unsigned char Temperature;//温度
extern unsigned char Humidity;//湿度
extern float Voltage; // 电压预设值，尚未接入 ADC 实测

extern unsigned char Relay1[];//1号继电器
extern unsigned char Relay2[];//2号继电器
extern unsigned char Relay3[];//3号继电器
extern unsigned char Relay4[];//4号继电器
extern unsigned char Relay5[];//5号继电器
/*-------------------MCU数据上传------------------*/








extern const unsigned char gImage_d[];
/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

extern u8 i,j;
extern float t;

extern uint8_t wu_t1_dat[];
extern uint8_t wu_r1_dat[100];

extern uint8_t wu_t2_dat[];
extern uint8_t wu_r2_dat[100];

extern uint8_t wu_t3_dat[];
extern uint8_t wu_r3_dat[100];

extern uint8_t wu_t4_dat[];
extern uint8_t wu_r4_dat[100];

extern uint8_t wu_t5_dat[];
extern uint8_t wu_r5_dat[100];

extern uint8_t wu_t6_dat[];
extern uint8_t wu_r6_dat[100];

extern uint8_t control[];

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

extern TIM_HandleTypeDef htim6;



extern UART_HandleTypeDef huart1;
extern UART_HandleTypeDef huart2;
extern UART_HandleTypeDef huart3;
extern UART_HandleTypeDef huart4;
extern UART_HandleTypeDef huart5;
extern UART_HandleTypeDef huart6;

extern DMA_HandleTypeDef hdma_usart1_rx;
extern DMA_HandleTypeDef hdma_usart1_tx;
extern DMA_HandleTypeDef hdma_usart2_rx;
extern DMA_HandleTypeDef hdma_usart2_tx;
extern DMA_HandleTypeDef hdma_usart3_rx;
extern DMA_HandleTypeDef hdma_usart3_tx;
extern DMA_HandleTypeDef hdma_uart4_rx;
extern DMA_HandleTypeDef hdma_uart4_tx;
extern DMA_HandleTypeDef hdma_uart5_rx;
extern DMA_HandleTypeDef hdma_uart5_tx;
extern DMA_HandleTypeDef hdma_usart6_rx;
extern DMA_HandleTypeDef hdma_usart6_tx;



/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define RELAY3_Pin GPIO_PIN_2
#define RELAY3_GPIO_Port GPIOE
#define WU_KEY3_CODE_Pin GPIO_PIN_2
#define WU_KEY3_CODE_GPIO_Port GPIOC
#define LED_BLUE_Pin GPIO_PIN_4
#define LED_BLUE_GPIO_Port GPIOA
#define LED_GREEN_Pin GPIO_PIN_5
#define LED_GREEN_GPIO_Port GPIOA
#define LED_RED_Pin GPIO_PIN_6
#define LED_RED_GPIO_Port GPIOA
#define SPI2_CS_Pin GPIO_PIN_12
#define SPI2_CS_GPIO_Port GPIOB
#define RELAY4_Pin GPIO_PIN_0
#define RELAY4_GPIO_Port GPIOD
#define RELAY5_Pin GPIO_PIN_1
#define RELAY5_GPIO_Port GPIOD
#define WU_KEY0_Pin GPIO_PIN_5
#define WU_KEY0_GPIO_Port GPIOD
#define WU_KEY1_Pin GPIO_PIN_6
#define WU_KEY1_GPIO_Port GPIOD
#define WU_KEY2_Pin GPIO_PIN_7
#define WU_KEY2_GPIO_Port GPIOD
#define OLED_SCL_Pin GPIO_PIN_6
#define OLED_SCL_GPIO_Port GPIOB
#define OLED_SDA_Pin GPIO_PIN_7
#define OLED_SDA_GPIO_Port GPIOB
#define BUZZER_Pin GPIO_PIN_8
#define BUZZER_GPIO_Port GPIOB
#define DHT11_Pin GPIO_PIN_9
#define DHT11_GPIO_Port GPIOB
#define RELAY1_Pin GPIO_PIN_0
#define RELAY1_GPIO_Port GPIOE
#define RELAY2_Pin GPIO_PIN_1
#define RELAY2_GPIO_Port GPIOE

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
