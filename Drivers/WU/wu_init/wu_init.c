/*
 * wu_init.c
 *
 *  Created on: Jul 16, 2024
 *      Author: 17076
 */
#include "wu_init.h"
#include "board.h"
#include <string.h>
#include "../wu_usart/wu_usart.h"


/************  **************************/


unsigned char Init_dat=0;


unsigned int T_5000ms;
unsigned char T_5000ms_flag_dat;

/************  **************************/


void WU_Init(void)
{
	LED(1);
	//BUZZER(1);
#if !defined(WU_USE_KK_UI)
	OLED_U8G2_Init();
#endif

	WU_CAN1_Init();


#if !defined(WU_USE_KK_UI)
	if(__HAL_RCC_GET_FLAG(RCC_FLAG_IWDGRST) != RESET)
	{
		WU_OLED_U8G2_ClearBuffer();

		WU_OLED_U8G2_SetFont(u8g2_font_wqy13_t_gb2312a);//选择字库
		WU_OLED_U8G2_Printf(20,20,CHINA,"看门狗复位");
		WU_OLED_U8G2_Printf(20,45,CHINA,"程序跑飞了。。。");

		WU_OLED_U8G2_SendBuffer();
	    __HAL_RCC_CLEAR_RESET_FLAGS();//清除复位标志
	}
	else
	{
		while(Init_dat<101)
		{
			WU_OLED_U8G2_ClearBuffer();

			WU_OLED_U8G2_SetFont(u8g2_font_wqy13_t_gb2312a);//选择字库

			WU_OLED_U8G2_Printf(20,20,DRAWSTRX2,"PATROL");
			WU_OLED_U8G2_DrawButtonUTF8(65, 35,  U8G2_BTN_SHADOW1|U8G2_BTN_HCENTER|U8G2_BTN_BW2, 0, 2, 2, "请等待初始化完成" );
			WU_OLED_U8G2_Printf(101,60,CHINA,"%d%%",Init_dat);
			WU_OLED_U8G2_DrawRFrame(0, 50, 100,10,4);
			WU_OLED_U8G2_DrawRBox(0, 50, Init_dat,10,4);

			WU_OLED_U8G2_SendBuffer();

			HAL_IWDG_Refresh(&hiwdg);//初始化喂狗
			Init_dat+=2;
		}
		WU_OLED_U8G2_DrawButtonUTF8(65, 35,  U8G2_BTN_SHADOW1|U8G2_BTN_HCENTER|U8G2_BTN_BW2|U8G2_BTN_INV, 0, 2, 2, "请等待初始化完成" );
		WU_OLED_U8G2_SendBuffer();
	}


#else
    /* KK_UI owns initialization and all display updates in its RTOS task. */
    __HAL_RCC_CLEAR_RESET_FLAGS();
#endif
	HAL_TIM_Base_Start_IT(&htim6);//使能定时器6定时中断

    HAL_TIM_Encoder_Start(&htim3,TIM_CHANNEL_ALL);//启动编码器
    htim3.Instance->CNT = 0;//给TIM3的CNT赋初值

	// 六路循环 DMA 接收一次启动，发送数据复制入队。
    Board_UartStart();
	Board_Send(0,wu_t1_dat, strlen((const char*)wu_t1_dat));

	Board_Send(1,wu_t2_dat, strlen((const char*)wu_t2_dat));

	Board_Send(2,wu_t3_dat, strlen((const char*)wu_t3_dat));

	Board_Send(3,wu_t4_dat, strlen((const char*)wu_t4_dat));

	Board_Send(4,wu_t5_dat, strlen((const char*)wu_t5_dat));

	Board_Send(5,wu_t6_dat, strlen((const char*)wu_t6_dat));


	DHT11_Init();//温湿度初始化
	BUZZER(1);
	//初始化喂狗
	HAL_IWDG_Refresh(&hiwdg);//初始化喂狗
	HAL_Delay(1000);
	LED(0);
	BUZZER(0);

	Seria2_Printf(
			"     \r\n"
			"            _ooOoo_\r\n"
			"           o8888888o\r\n"
			"           88\" . \"88\r\n"
			"           (| -_- |)\r\n"
			"            O\\ = /O\r\n"
			"        ____/`---'\\____\r\n"
			"      .   ' \\\\| |// `.\r\n"
			"       / \\\\||| : |||// \\\r\n"
			"     / _||||| -:- |||||- \\\r\n"
			"       | | \\\\\\ - /// | |\r\n"
			"     | \\_| ''\\---/'' | |\r\n"
			"      \\ .-\\__ `-` ___/-. /\r\n"
			"   ___`. .' /--.--\\ `. . __\r\n"
			"    .\"\" '< `.___\\_<|>_/___.' >'\"\".\r\n"
			"   | | : `- \\`.;`\\ _ /`;.`/ - ` : | |\r\n"
			"     \\ \\ `-. \\_ __\\ /__ _/ .-` / /\r\n"
			"======`-.____`-._____\\_____/___.-`____.-'======\r\n"
			"            `=-2025-='\r\n");


 }

/*任务进度条百分%至*/
void Count_Task_Init(void)
{
	count = __HAL_TIM_GET_COUNTER(&htim3);
	if(count>6000)
	{
		count=0;
		__HAL_TIM_SET_COUNTER(&htim3,0);
	}
	else if(count>100)
	{
		count=100;
		__HAL_TIM_SET_COUNTER(&htim3,100);
	}
}


/*任务进度条*/
void Usart_Task_Init(void)
{
	count = __HAL_TIM_GET_COUNTER(&htim3);
	if(count>6000)
	{
		count=0;
		__HAL_TIM_SET_COUNTER(&htim3,0);
	}
	else if(count>6)
	{
		count=6;
		__HAL_TIM_SET_COUNTER(&htim3,6);
	}
}



















