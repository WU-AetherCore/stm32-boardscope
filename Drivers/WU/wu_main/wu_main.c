/*
 * wu_main.c
 *
 *  Created on: Jun 25, 2024
 *      Author: wu
 */
#include "wu_main.h"
#include "board.h"
#include "../../WU/wu_init/wu_init.h"
#include "../../WU/callback/callback.h"
#include "../../WU/wu_usart/wu_usart.h"

/*-------------------MCU数据上传------------------*/

unsigned char T_alarm;//温度报警标志位
unsigned char Temperature_max=45;//上限温度
unsigned char Temperature;//温度
unsigned char Humidity;//湿度
float Voltage=11.82;//锂电池电压

unsigned char Relay1[]="relay1_nil";//1号继电器
unsigned char Relay2[]="relay2_nil";//2号继电器
unsigned char Relay3[]="relay3_nil";//3号继电器
unsigned char Relay4[]="relay4_nil";//4号继电器
unsigned char Relay5[]="relay5_nil";//5号继电器
/*-------------------MCU数据上传------------------*/


unsigned int cs_bj_dat;


/**************************************************************************
函数功能：浮点型数据计算绝对值
入口参数：浮点数
返回  值：输入数的绝对值
**************************************************************************/
float float_abs(float insert)
{
	if(insert>=0) return insert;
	else return -insert;
}

int wu_absolute_value(int wu_dat)
{
	if(wu_dat<0)
	{
		return -wu_dat;
	}
	else
		return wu_dat;
}







/* 兼容旧业务调用：只上传真实基础数据，不再夹带演示用固定数值。 */
void Data_Upload(void)
{
    Seria2_Printf("T=%u H=%u LIMIT=%u ALARM=%u R=%u%u%u%u%u\r\n",
        Temperature, Humidity, Temperature_max, T_alarm,
        relay1_flag_dat, relay2_flag_dat, relay3_flag_dat, relay4_flag_dat, relay5_flag_dat);
}

void Usart_Control(void)//串口控制
{
	if(wu_r2_dat[0]=='F')
	{

		if(wu_r2_dat[1]=='a')//继电器控制指令
		{
			switch(wu_r2_dat[2])
			{
				case '0':relay1_flag_dat=0;break;
				case '1':relay1_flag_dat=1;break;

				case '2':relay2_flag_dat=0;break;
				case '3':relay2_flag_dat=1;break;

				case '4':relay3_flag_dat=0;break;
				case '5':relay3_flag_dat=1;break;

				case '6':relay4_flag_dat=0;break;
				case '7':relay4_flag_dat=1;break;

				case '8':relay5_flag_dat=0;break;
				case '9':relay5_flag_dat=1;break;
			}
		}



		if(wu_r2_dat[1]=='b')//语音控制指令
		{
			switch(wu_r2_dat[2])
			{
				case 'A':
					led_sgd=0;
				break;
				case 'B':
					led_sgd=1;
				break;
			}
		}



	}
}







void MAIN_WUStartTask05(void *argument)
{

	osTimerStart(WUTimer01Handle, 2000);
	while(1)
	{
		/* Commands must be consumed once; V1.1 UART1 uses newline commands. */

		T_alarm = !cs_bj_flag_bit && board_dht_ok && Temperature >= Temperature_max;
        LED_RED(T_alarm); LED_GREEN(!T_alarm);

		osDelay(1);
	}
}





void Callback01(void *argument)
{
	//LED_GREEN_TogglePin;

	board_dht_ok = DHT11_Read_Data(&Temperature, &Humidity) == 0;
    if(board_dht_ok) board_dht_last=HAL_GetTick(); // 读取温湿度

	/* UI and UART STATUS provide real data. */

	if(++cs_bj_dat==100)cs_bj_dat=0;
}














