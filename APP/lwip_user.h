#ifndef _LWIP_USER
#define _LWIP_USER

#include "stm32f10x.h"
typedef struct
{
	u8 Mac[6];
	u8 IP[4];
	u8 Mask[4];
	u8 GW[4];
	u8 dhcpstatus;
}IP_Config;

u8 lwip_config_init(void);
void lwip_periodic_handle(void);
void lwip_dhcp_process_handle(void);

#endif

