#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include <uart.h>
#include "osapi.h"
#include "espconn.h"
#include "mystuff.h"

#define procTaskPrio        0
#define procTaskQueueLen    1


static volatile os_timer_t some_timer;

//Tasks that happen all the time.
os_event_t    procTaskQueue[procTaskQueueLen];
static void ICACHE_FLASH_ATTR
procTask(os_event_t *events)
{
	system_os_post(procTaskPrio, 0, 0 );
	if( events->sig == 0 && events->par == 0 )
	{
		//Idle Event.
	}
}


static void packetcb(uint8_t *buf, uint16 len)
{
	char ct[32];
	ets_sprintf( ct, ">%p %d\n", buf, len );
	uart0_sendStr( ct );
}

//Timer event.
static void  myTimer(void *arg)
{
	int i;
	uart0_sendStr(".");

	char buffer[20];
	buffer[0] = 0xfc;
	buffer[1] = 0;
	buffer[2] = 0;
	buffer[3] = 0;
	ets_strcpy( buffer+4, "Hello, world!!!\n", 16 );

	ets_intr_lock();
	RawSendBuffer( buffer, 20 );
	ets_intr_unlock();

}


void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	int wifiMode = wifi_get_opmode();

	uart0_sendStr("\r\nCustom Server\r\n");

	wifi_set_opmode( 2 ); //We broadcast our ESSID, wait for peopel to join.

	wifi_station_dhcpc_start();

	//XXX TODO figure out how to safely re-allow this.
	ets_wdt_disable();

	//Add a process
	system_os_task(procTask, procTaskPrio, procTaskQueue, procTaskQueueLen);

	uart0_sendStr("\r\nCustom Server\r\n");

	//I want to experiment here.
	wifi_promiscuous_enable(0);
	wifi_set_promiscuous_rx_cb( packetcb );

	//For sending raw packets.
	SetupRawsend();

	//Timer example
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)myTimer, NULL);
	os_timer_arm(&some_timer, 50, 1);
 
	system_os_post(procTaskPrio, 0, 0 );
}


