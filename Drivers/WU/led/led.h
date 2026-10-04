/*
 * led.h
 *
 *  Created on: Apr 1, 2024
 *      Author: wu
 */

#ifndef WU_LED_LED_H_
#define WU_LED_LED_H_

#include "main.h"
#include "../../WU/key/key.h"


extern unsigned char relay1_flag_dat,relay2_flag_dat,relay3_flag_dat,relay4_flag_dat,relay5_flag_dat;
extern unsigned char buzzer_flag_dat,buzzer_flag_dat1;



#define LED_RED(x) x?HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_RESET):\
		             HAL_GPIO_WritePin(LED_RED_GPIO_Port, LED_RED_Pin, GPIO_PIN_SET);

#define LED_GREEN(x) x?HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_RESET):\
		               HAL_GPIO_WritePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin, GPIO_PIN_SET);

#define LED_BLUE(x) x?HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_RESET):\
		              HAL_GPIO_WritePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin, GPIO_PIN_SET);


#define LED_RED_TogglePin  HAL_GPIO_TogglePin(LED_RED_GPIO_Port, LED_RED_Pin);
#define LED_GREEN_TogglePin  HAL_GPIO_TogglePin(LED_GREEN_GPIO_Port, LED_GREEN_Pin);
#define LED_BLUE_TogglePin  HAL_GPIO_TogglePin(LED_BLUE_GPIO_Port, LED_BLUE_Pin);


#define BUZZER(x) x?HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET):\
		            HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

#define RELAY1(x) x?HAL_GPIO_WritePin(RELAY1_GPIO_Port, RELAY1_Pin, GPIO_PIN_RESET):\
		            HAL_GPIO_WritePin(RELAY1_GPIO_Port, RELAY1_Pin, GPIO_PIN_SET);

#define RELAY2(x) x?HAL_GPIO_WritePin(RELAY2_GPIO_Port, RELAY2_Pin, GPIO_PIN_RESET):\
		            HAL_GPIO_WritePin(RELAY2_GPIO_Port, RELAY2_Pin, GPIO_PIN_SET);

#define RELAY3(x) x?HAL_GPIO_WritePin(RELAY3_GPIO_Port, RELAY3_Pin, GPIO_PIN_RESET):\
		            HAL_GPIO_WritePin(RELAY3_GPIO_Port, RELAY3_Pin, GPIO_PIN_SET);

#define RELAY4(x) x?HAL_GPIO_WritePin(RELAY4_GPIO_Port, RELAY4_Pin, GPIO_PIN_RESET):\
		            HAL_GPIO_WritePin(RELAY4_GPIO_Port, RELAY4_Pin, GPIO_PIN_SET);

#define RELAY5(x) x?HAL_GPIO_WritePin(RELAY5_GPIO_Port, RELAY5_Pin, GPIO_PIN_RESET):\
		            HAL_GPIO_WritePin(RELAY5_GPIO_Port, RELAY5_Pin, GPIO_PIN_SET);


void LED(u8 x);


#endif /* WU_LED_LED_H_ */


