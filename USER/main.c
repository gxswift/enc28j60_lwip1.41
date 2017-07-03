#include "stm32f10x.h"
#include "led.h"
#include "timer.h"
#include "enc28j60.h"
#include "lwip_user.h"
#include "delay.h"


extern LED Led;
u32 lwip_localtime;	
u8 temp;
 int main(void)
 {	u32 t = 0;
	 delay_init();	 
	 LED_Init();
	 TIM3_Int_Init(1000,71);
	 temp = lwip_config_init();
 #if LWIP_DHCP   //使用DHCP
	while((lwipdev.dhcpstatus!=2)&&(lwipdev.dhcpstatus!=0XFF))//等待DHCP获取成功/超时溢出
	{
		lwip_periodic_handle();	//LWIP内核需要定时处理的函数
	}
#endif
	while(1)
	{	
		lwip_periodic_handle();	//LWIP内核需要定时处理的函数
	/*	t++;
		if(t == 500000)
		{
			t = 0;
			Led.L1 ^= 1;

			Led.L2 ^= 1;
	
			Led_Fun(Led);
		}*/

	}
 }

