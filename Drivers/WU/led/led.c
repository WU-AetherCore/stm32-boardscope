/*
 * led.c
 *
 *  Created on: Apr 1, 2024
 *      Author: wu
 */
#include"led.h"
#include "cmsis_os.h"
#include <string.h>

unsigned char relay1_flag_dat,relay2_flag_dat,relay3_flag_dat,relay4_flag_dat,relay5_flag_dat;
unsigned char buzzer_flag_dat,buzzer_flag_dat1;


void LED(u8 x)
{
	if(x==1)
	{
		LED_RED(1);
		LED_GREEN(1);
		LED_BLUE(1);
	}else
	{
		LED_RED(0);
		LED_GREEN(0);
		LED_BLUE(0);
	}
}


void Wu_Led_Proc(void)//LED相关函数
{
	if(key_dat_2)buzzer_flag_dat=1;
	else buzzer_flag_dat=0;
	//HAL_GPIO_TogglePin(LED_RED_GPIO_Port,LED_RED_Pin);
	BUZZER(buzzer_flag_dat | buzzer_flag_dat1 | T_alarm);
	//LED_RED(led_sgd);
	RELAY1(relay1_flag_dat);
	RELAY2(relay2_flag_dat);
	RELAY3(relay3_flag_dat);
	RELAY4(relay4_flag_dat);
	RELAY5(relay5_flag_dat);


	//	if(Temperature>Temperature_max)
	//	{
	//		buzzer_flag_dat1=1;
	//		T_alarm=1;
	//	}else
	//	{
	//		buzzer_flag_dat1=0;
	//		T_alarm=0;
	//	}

	relay1_flag_dat?strcpy((char*)Relay1,"relay1_ON"):strcpy((char*)Relay1,"relay1_OFF");
	relay2_flag_dat?strcpy((char*)Relay2,"relay2_ON"):strcpy((char*)Relay2,"relay2_OFF");
	relay3_flag_dat?strcpy((char*)Relay3,"relay3_ON"):strcpy((char*)Relay3,"relay3_OFF");
	relay4_flag_dat?strcpy((char*)Relay4,"relay4_ON"):strcpy((char*)Relay4,"relay4_OFF");
	relay5_flag_dat?strcpy((char*)Relay5,"relay5_ON"):strcpy((char*)Relay5,"relay5_OFF");

}


void LED_WUStartTask02(void *argument)
{
	while(1)
	{
		Wu_Led_Proc();
        osDelay(5);/*------------------------------------------------LED**相关函数*/
	}
}


