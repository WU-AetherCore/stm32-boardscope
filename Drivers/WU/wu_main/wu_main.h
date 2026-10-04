/*
 * wu_main.h
 *
 *  Created on: Jun 25, 2024
 *      Author: wu
 */

#ifndef WU_WU_MAIN_WU_MAIN_H_
#define WU_WU_MAIN_WU_MAIN_H_


#include "main.h"

#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "cmsis_os.h"

#include "../../WU/led/led.h"
#include "../../WU/key/key.h"
#include "../../WU/DHT11/DHT11.h"


extern osTimerId WUTimer01Handle;


extern unsigned int cs_bj_dat;




void Data_Upload(void);
void Usart_Control(void);
















#endif /* WU_WU_MAIN_WU_MAIN_H_ */
