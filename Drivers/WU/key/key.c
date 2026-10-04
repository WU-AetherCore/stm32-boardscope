/*
 * key.c
 *
 *  Created on: Apr 1, 2024
 *      Author: wu
 */
#include "key.h"
#include "cmsis_os.h"
#include "board.h"

unsigned char Key_Down,Key_Up,Key_Val,Key_Old;
unsigned char T_10ms;
unsigned char Key_Flag_Bit;

unsigned char key_dat_1;//双击
unsigned char key_dat_2;//长按
unsigned char key_dat_3;//短按

unsigned int Key_Cnt_Flag = 0;//
unsigned char key_flag;

unsigned char y;
unsigned char oled_interface_flag_dat;//OLED界面
unsigned char key4_dat;

unsigned char cs_bj_flag_bit=1;

unsigned char Key_Read()
{
	uint16_t temp=0;

	if(WU_KEY0 == 0)temp=1;
	if(WU_KEY1 == 0)temp=2;
	if(WU_KEY2 == 0)temp=3;
	if(WU_KEY3_CODE == 0)temp=4;

	return temp;
}


void Wu_Key_Proc(void)
{
	if(Key_Flag_Bit!=1)return;
	Key_Flag_Bit=0;
	Key_Val = Key_Read();//实时读取键码
	Key_Down = Key_Val & (Key_Old ^ Key_Val);//捕捉按键下降
	Key_Up = ~Key_Val & (Key_Old ^ Key_Val);//捕捉按键上升
	Key_Old = Key_Val;//辅助扫描变量

	switch(Key_Down)
	{
	case 1:

		relay1_flag_dat^=1;

		//y=1;
		if(Key_Cnt_Flag){	//先进行判断双击动
			//执行双击
			key_dat_1^=1;
			Key_Cnt_Flag = 0;	//复位
		}else{
			Key_Cnt_Flag = 1;	//
		}



		TxData[0] ++;
		TxData[1] ++;
		TxData[2] ++;
		TxData[3] ++;

		WU_CAN_T(TxID, TxLength, TxData);


		break;
	case 2:

		/* 发送示例 */
		WU_CAN_T(TxID, TxLength, (uint8_t*)"wwwwijfie");

		relay2_flag_dat^=1;

		//led_sgd^=1;
		y=5;
		break;
	case 3:
		cs_bj_flag_bit^=1;

		if(++oled_interface_flag_dat==4)oled_interface_flag_dat=0;
		break;
	case 4:
		key4_dat^=1;
		break;
	}
}



void KEY_WUStartTask03(void const *argument)
{
	while(1)
	{
#if defined(WU_USE_KK_UI)
        Board_KeyScan(HAL_GetTick());
        osDelay(10U);
#else
		Wu_Key_Proc();/*------------------------------------------------按键*相关函数-*/
#endif
	}
}










