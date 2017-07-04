#include "lwip_user.h" 
#include "netif/etharp.h"
#include "lwip/dhcp.h"
#include "lwip/mem.h"
#include "lwip/memp.h"
#include "lwip/init.h"
#include "ethernetif.h" 
#include "lwip/timers.h"
#include "lwip/tcp_impl.h"
#include "lwip/ip_frag.h"
#include "lwip/tcpip.h" 
#include "enc28j60.h"
#include "ethernetif.h"
#include <stdio.h>

#define LWIP_MAX_DHCP_TRIES 4
IP_Config lwipdev;						//lwip控制结构体 
struct netif lwip_netif;				//定义一个全局的网络接口

extern u32 lwip_localtime;		//lwip本地时间计数器,单位:ms

u32 TCPTimer=0;			//TCP查询计时器
u32 ARPTimer=0;			//ARP查询计时器
u32 InputTimer = 0;

u8 Mac[6] = {0x04,0x02,0x35,0x00,0x00,0x01};

#if LWIP_DHCP
u32 DHCPfineTimer=0;	//DHCP精细处理计时器
u32 DHCPcoarseTimer=0;	//DHCP粗糙处理计时器
#endif

//lwip 默认IP设置
//lwipx:lwip控制结构体指针
void lwip_ip_config(IP_Config *lwipx)
{
//	//默认远端IP为:192.168.1.100
//	lwipx->remoteip[0]=192;	
//	lwipx->remoteip[1]=168;
//	lwipx->remoteip[2]=1;
//	lwipx->remoteip[3]=100;
	//MAC地址设置(高三字节固定为:2.0.0,低三字节用STM32唯一ID)
	lwipx->Mac[0]=Mac[0];
	lwipx->Mac[1]=Mac[1];
	lwipx->Mac[2]=Mac[2];
	lwipx->Mac[3]=Mac[3];
	lwipx->Mac[4]=Mac[4];
	lwipx->Mac[5]=Mac[5]; 
	//默认本地IP为:192.168.1.30
	lwipx->IP[0]=192;	
	lwipx->IP[1]=168;
	lwipx->IP[2]=1;
	lwipx->IP[3]=16;
	//默认子网掩码:255.255.255.0
	lwipx->Mask[0]=255;	
	lwipx->Mask[1]=255;
	lwipx->Mask[2]=255;
	lwipx->Mask[3]=0;
	//默认网关:192.168.1.1
	lwipx->GW[0]=192;	
	lwipx->GW[1]=168;
	lwipx->GW[2]=1;
	lwipx->GW[3]=1;	
//	lwipx->GW=0;//没有DHCP	
	lwipx->dhcpstatus = 0;
} 

//LWIP初始化(LWIP启动的时候使用)
//返回值:0,成功
//      1,内存错误
//      2,DM9000初始化失败
//      3,网卡添加失败.
u8 lwip_config_init(void)
{
	struct netif *Netif_Init_Flag;		//调用netif_add()函数时的返回值,用于判断网络初始化是否成功
	struct ip_addr ipaddr;  			//ip地址
	struct ip_addr netmask; 			//子网掩码
	struct ip_addr gw;      			//默认网关 
	mem_init();
	memp_init();
	if(ENC28J60_Init((u8*)Mac))return 1;		//初始化ENC28J60
	lwip_init();						//初始化LWIP内核
	lwip_ip_config(&lwipdev);	//设置默认IP等信息

#if LWIP_DHCP		//使用动态IP
	ipaddr.addr = 0;
	netmask.addr = 0;
	gw.addr = 0;
#else				//使用静态IP
	IP4_ADDR(&ipaddr,lwipdev.IP[0],lwipdev.IP[1],lwipdev.IP[2],lwipdev.IP[3]);
	IP4_ADDR(&netmask,lwipdev.Mask[0],lwipdev.Mask[1] ,lwipdev.Mask[2],lwipdev.Mask[3]);
	IP4_ADDR(&gw,lwipdev.GW[0],lwipdev.GW[1],lwipdev.GW[2],lwipdev.GW[3]);
#endif
	Netif_Init_Flag=netif_add(&lwip_netif,&ipaddr,&netmask,&gw,NULL,&ethernetif_init,&ethernet_input);//向网卡列表中添加一个网口
	
#if LWIP_DHCP			//如果使用DHCP的话
	lwipdev.dhcpstatus=0;	//DHCP标记为0
	dhcp_start(&lwip_netif);	//开启DHCP服务
#endif
	if(Netif_Init_Flag==NULL)return 3;//网卡添加失败 
	else//网口添加成功后,设置netif为默认值,并且打开netif网口
	{
		netif_set_default(&lwip_netif); //设置netif为默认网口
		netif_set_up(&lwip_netif);		//打开netif网口
	}
	return 0;//操作OK.
}   

//当接收到数据后调用 
void lwip_pkt_handle(void)
{
	//从网络缓冲区中读取接收到的数据包并将其发送给LWIP处理 
	ethernetif_input(&lwip_netif);
}

//LWIP轮询任务
void lwip_periodic_handle(void)
{
	if(lwip_localtime - InputTimer >2)
	{
		InputTimer =  lwip_localtime;
		ethernetif_input(&lwip_netif );
	}	
	//ARP每5s周期性调用一次
	if ((lwip_localtime - ARPTimer) >= ARP_TMR_INTERVAL)
	{
		ARPTimer =  lwip_localtime;
		etharp_tmr();
	}

#if LWIP_TCP
	//每250ms调用一次tcp_tmr()函数
	if (lwip_localtime - TCPTimer >= TCP_TMR_INTERVAL)
	{
		TCPTimer =  lwip_localtime;
		tcp_tmr();
	}
#endif

#if LWIP_DHCP //如果使用DHCP的话
	//每500ms调用一次dhcp_fine_tmr()
	if (lwip_localtime - DHCPfineTimer >= DHCP_FINE_TIMER_MSECS)
	{
		DHCPfineTimer =  lwip_localtime;
		dhcp_fine_tmr();
		if ((lwipdev.dhcpstatus != 2)&&(lwipdev.dhcpstatus != 0XFF))
		{ 
			lwip_dhcp_process_handle();  //DHCP处理
		}
	}

	//每60s执行一次DHCP粗糙处理
	if (lwip_localtime - DHCPcoarseTimer >= DHCP_COARSE_TIMER_MSECS)
	{
		DHCPcoarseTimer =  lwip_localtime;
		dhcp_coarse_tmr();
	}  
#endif
}


//如果使能了DHCP
#if LWIP_DHCP

//DHCP处理任务
void lwip_dhcp_process_handle(void)
{
	u32 ip=0,netmask=0,gw=0;
	switch(lwipdev.dhcpstatus)
	{
		case 0: 	//开启DHCP
			dhcp_start(&lwip_netif);
			lwipdev.dhcpstatus = 1;		//等待通过DHCP获取到的地址
			break;
		case 1:		//等待获取到IP地址
		{
			ip=lwip_netif.ip_addr.addr;		//读取新IP地址
			netmask=lwip_netif.netmask.addr;//读取子网掩码
			gw=lwip_netif.gw.addr;			//读取默认网关 
			
			if(ip!=0)			//正确获取到IP地址的时候
			{
				lwipdev.dhcpstatus=2;	//DHCP成功
				//解析出通过DHCP获取到的IP地址
				lwipdev.IP[3]=(uint8_t)(ip>>24); 
				lwipdev.IP[2]=(uint8_t)(ip>>16);
				lwipdev.IP[1]=(uint8_t)(ip>>8);
				lwipdev.IP[0]=(uint8_t)(ip);
				//解析通过DHCP获取到的子网掩码地址
				lwipdev.Mask[3]=(uint8_t)(netmask>>24);
				lwipdev.Mask[2]=(uint8_t)(netmask>>16);
				lwipdev.Mask[1]=(uint8_t)(netmask>>8);
				lwipdev.Mask[0]=(uint8_t)(netmask);
				//解析出通过DHCP获取到的默认网关
				lwipdev.GW[3]=(uint8_t)(gw>>24);
				lwipdev.GW[2]=(uint8_t)(gw>>16);
				lwipdev.GW[1]=(uint8_t)(gw>>8);
				lwipdev.GW[0]=(uint8_t)(gw);
			}else if(lwip_netif.dhcp->tries>LWIP_MAX_DHCP_TRIES) //通过DHCP服务获取IP地址失败,且超过最大尝试次数
			{
				lwipdev.dhcpstatus=0XFF;//DHCP超时失败.
				//使用静态IP地址
				IP4_ADDR(&(lwip_netif.ip_addr),lwipdev.IP[0],lwipdev.IP[1],lwipdev.IP[2],lwipdev.IP[3]);
				IP4_ADDR(&(lwip_netif.netmask),lwipdev.Mask[0],lwipdev.Mask[1],lwipdev.Mask[2],lwipdev.Mask[3]);
				IP4_ADDR(&(lwip_netif.gw),lwipdev.GW[0],lwipdev.GW[1],lwipdev.GW[2],lwipdev.GW[3]);
			}
		}
		break;
		default : break;
	}
}
#endif 


u32_t sys_now(void){
	return lwip_localtime;
}


