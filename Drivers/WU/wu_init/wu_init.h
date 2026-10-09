/*
 * wu_init.h
 *
 *  Created on: Jul 16, 2024
 *      Author: 17076
 */

#ifndef WU_WU_INIT_WU_INIT_H_
#define WU_WU_INIT_WU_INIT_H_


#include "main.h"

#include "../../WU/led/led.h"
#include "../../WU/key/key.h"
#include "../../WU/wu_main/wu_main.h"

#if !defined(WU_USE_ASTRA_UI)
#include "../../WU/OLED_U8g2/IIC_OLED/UI_OLED.h"
#endif

#include "../../WU/wu_can/wu_can.h"

/*------------------------------------------------------------------------------------------------------------------------------*/




/*------------------------------------------------------------------------------------------------------------------------------*/


void WU_Init(void);

void Count_Task_Init(void);
void Usart_Task_Init(void);



#endif /* WU_WU_INIT_WU_INIT_H_ */

