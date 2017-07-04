#ifndef __LED_H
#define __LED_H	 
#include "stm32f10x.h"

typedef struct
{
	u8 L1:1;
	u8 L2:1;
}LED;
	
	
void LED_Init(void);//≥ı ºªØ
void Led_Fun(LED *led);
		 				    
#endif
