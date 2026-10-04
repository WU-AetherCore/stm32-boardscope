/* USER CODE BEGIN Header */
/**
 ******************************************************************************
 * @file           : main.c
 * @brief          : Main program body
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
/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "cmsis_os.h"
#include "can.h"
#include "dma.h"
#include "iwdg.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

#include "string.h"
#include "stdio.h"
#include "../../WU/led/led.h"
#include "../../WU/key/key.h"
#include "../../WU/DHT11/DHT11.h"
#include "../../WU/callback/callback.h"
#include "../../WU/wu_main/wu_main.h"
#include "../../WU/wu_init/wu_init.h"
#include "../../WU/wu_usart/wu_usart.h"
#include "../../WU/W25Q/W25Q.h"
#include "../../WU/wu_can/wu_can.h"

/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

u8 i,j;
float t=0;


//int rest_dat=0;

uint8_t rising_flag=0;
uint8_t falling_flag=0;
uint8_t overflow_val=0;
uint16_t cnt_val=0;

uint32_t temp=0;



/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

unsigned char led_sgd;
unsigned char buz_sgd;
unsigned char Led_Flag_Bit;
unsigned int T_100ms;

unsigned char at_Flag_Bit;
unsigned int T_800ms;

unsigned int T_1500ms,T_1500ms_flag_dat;

unsigned char oled_xs_dat=0;

unsigned char t_dat=1;


unsigned char T_20ms;
unsigned char T_20ms_flag_dat;




unsigned char ScrollDisplay_dat;


/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

uint8_t wu_t1_dat[]="Hello,Usart1!";
uint8_t wu_r1_dat[100];

uint8_t wu_t2_dat[]="Hello,Usart2!";
uint8_t wu_r2_dat[100];

uint8_t wu_t3_dat[]="Hello,Usart3!";
uint8_t wu_r3_dat[100];

uint8_t wu_t4_dat[]="Hello,Usart4!";
uint8_t wu_r4_dat[100];

uint8_t wu_t5_dat[]="Hello,Usart5!";
uint8_t wu_r5_dat[100];

uint8_t wu_t6_dat[]="Hello,Usart6!";
uint8_t wu_r6_dat[100];

uint8_t control[] = "Z";


/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/

/* USER CODE BEGIN PV */





/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */



/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */



  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_DMA_Init();
  MX_USART1_UART_Init();
  MX_USART3_UART_Init();
  MX_TIM3_Init();
  MX_TIM6_Init();
  MX_IWDG_Init();
  MX_USART2_UART_Init();
  MX_UART4_Init();
  MX_UART5_Init();
  MX_USART6_UART_Init();
  MX_SPI2_Init();
  MX_CAN1_Init();
  /* USER CODE BEGIN 2 */
	/*======================================================================================================================================================*/
	WU_Init();//------------------------------------------------------/*个人硬解初始*/



	unsigned char led_dat1=0,led_dat2=0;
	/*======================================================================================================================================================*/
  /* USER CODE END 2 */

  /* Call init function for freertos objects (in cmsis_os2.c) */
  MX_FREERTOS_Init();

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
	while (1)
	{
		/******************************************************************************************************************************************************/
//		LED_GREEN(led_dat1==led_dat2);
//		//HAL_Delay(3500);
//		/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
//		Wu_xsp_Proc();/*------------------------------------------------显示屏相关函*/
//		Wu_Led_Proc();/*------------------------------------------------LED**相关函数*/
//		Wu_Key_Proc();/*------------------------------------------------按键*相关函数-*/
//		Wu_Mai_Proc();/*------------------------------------------------主要*相关函数-*/
//		/*----------------------------------------------------------------------------------------------------------------------------------------------------*/
//		led_dat1=1;
//		HAL_IWDG_Refresh(&hiwdg);//-------------------------------------喂狗
		/******************************************************************************************************************************************************/
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
	}
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI|RCC_OSCILLATORTYPE_LSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.LSIState = RCC_LSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSI;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2) {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  	if(htim == &htim6){



  		if(++T_10ms==10){
  			T_10ms=0;
  			Key_Flag_Bit=1;
  		}

  		if(++T_100ms==500){
  			T_100ms=0;
  			Led_Flag_Bit^=1;
  		}

  		LED_BLUE(Led_Flag_Bit);//工作指示闪光�??

  		if(++T_800ms==800){
  			T_800ms=0;
  			at_Flag_Bit^=1;
  		}


  		if(++T_1500ms==1500)
  		{
  			T_1500ms=0;
  			T_1500ms_flag_dat^=1;
  			HAL_IWDG_Refresh(&hiwdg);//-------------------------------------喂狗
  		}

  		if(Key_Cnt_Flag){				//长按计时部分
  			if(++Key_Cnt_Flag==1000){	//以认
  			Key_Cnt_Flag = 0;
  			if(Key_Val){	//没松
  				key_dat_2^=1;//执行长按动作

  				}else{
  					key_dat_3^=1;//执行短按动作

  				}
  			}
  		}


  		if(++T_5000ms==5000)
  		{
  			T_5000ms=0;
  			if(++T_5000ms_flag_dat==4)T_5000ms_flag_dat=0;
  		}


  	}

  /* USER CODE END Callback 1 */
}

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
	/* User can add his own implementation to report the HAL error return state */
	__disable_irq();
	while (1)
	{
	}
  /* USER CODE END Error_Handler_Debug */
}

#ifdef  USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
	/* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
