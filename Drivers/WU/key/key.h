/*
 * key.h
 *
 *  Created on: Apr 1, 2024
 *      Author: wu
 */

#ifndef WU_KEY_KEY_H_
#define WU_KEY_KEY_H_

#include "main.h"
#include "../../WU/wu_can/wu_can.h"



extern unsigned char Key_Down,Key_Up,Key_Val,Key_Old;
extern unsigned char T_10ms;
extern unsigned char Key_Flag_Bit;

extern unsigned char key_dat_1;//双击
extern unsigned char key_dat_2;//长按
extern unsigned char key_dat_3;//短按

extern unsigned int Key_Cnt_Flag;
extern unsigned char key_flag;

extern unsigned char oled_interface_flag_dat;//OLED界面
extern unsigned char key4_dat;

extern unsigned char cs_bj_flag_bit;


#define WU_KEY0 HAL_GPIO_ReadPin(WU_KEY0_GPIO_Port,WU_KEY0_Pin)
#define WU_KEY1 HAL_GPIO_ReadPin(WU_KEY1_GPIO_Port,WU_KEY1_Pin)
#define WU_KEY2 HAL_GPIO_ReadPin(WU_KEY2_GPIO_Port,WU_KEY2_Pin)

#define WU_KEY3_CODE HAL_GPIO_ReadPin(WU_KEY3_CODE_GPIO_Port,WU_KEY3_CODE_Pin)

#define W_D3 HAL_GPIO_ReadPin(GPIOD,GPIO_PIN_3)


unsigned char Key_Read();

#endif /* WU_KEY_KEY_H_ */
