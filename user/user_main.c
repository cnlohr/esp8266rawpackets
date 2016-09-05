#include "mem.h"
#include "c_types.h"
#include "user_interface.h"
#include "ets_sys.h"
#include <uart.h>
#include "osapi.h"
#include "espconn.h"
#include "mystuff.h"
#include "esp_rawsend.h"

#define procTaskPrio        0
#define procTaskQueueLen    1

char generic_print_buffer[384];

static struct espconn *pUdpServer;

static volatile os_timer_t some_timer;

//Tasks that happen all the time.
os_event_t    procTaskQueue[procTaskQueueLen];
static void ICACHE_FLASH_ATTR
procTask(os_event_t *events)
{
	system_os_post(procTaskPrio, 0, 0 );

	CSTick( 0 );

	if( events->sig == 0 && events->par == 0 )
	{
		//Idle Event.
	}
}


void udpserver_recv(void *arg, char *pusrdata, unsigned short len)
{
	struct espconn *pespconn = (struct espconn *)arg;
	//Not used.
}

void ICACHE_FLASH_ATTR rx_func( struct RxPacket * r )
{
	if( r->data[24] != 0x82 || r->data[25] != 0x66 )
	{
		return;
	}

	espconn_sent(pUdpServer, r, r->rx_ctl.legacy_length + 8 ); //+8 = include header

/*
	//printf( ">%d<\n", r->data[0] );
	int i;
	for( i = 0;  i < 10; i++ )
	{
		printf( "%02x ", r->data[i+24] );
	}
	printf( "\n" );	
*/
}


uint8_t mypacket[30+256] = {  //256 = max size of additional payload
	0x08, //Frame type, 0x80 = beacon, Tried data, but seems to have been filtered on RX side by other ESP
	0x00, 0x00, 0x00, 
	0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,
	0xff,0xff,0xff,0xff,0xff,0xff,
	0x00, 0x00,  //Sequence number, cleared by espressif
	0x82, 0x66,	 //"Mysterious OLPC stuff"
	0x00, 0x00, 0x00, 0x00, //????
	
};

//Timer event.
extern uint8_t printed_ip;
extern uint8_t * wDevCtrl;
static void  myTimer(void *arg)
{
	int i;
	static int did_init = 0;

	CSTick( 1 );

	if( !did_init && printed_ip )
	{
		//For sending raw packets.
		//SetupRawsend();
		wifi_set_raw_recv_cb( rx_func );
		did_init = 1;

		//Setup our send packet with our MAC address.
		wifi_get_macaddr(STATION_IF, mypacket + 10);
	}


	if( did_init )
	{
		uart0_sendStr(".");
		ets_strcpy( mypacket+30, "Hello, world" );
		wifi_send_pkt_freedom( mypacket, 30 + 12, true) ;  //Currently only seems to send at 1MBit/s.  Need to figure out how to bump that up.

/*
	//Not doing this (yet)
		ets_intr_lock();
		RawSendBuffer( mypacket, 20 );
		ets_intr_unlock();
*/
	}

}

void ICACHE_FLASH_ATTR charrx( uint8_t c ) {/*Called from UART.*/}

void user_init(void)
{
	uart_init(BIT_RATE_115200, BIT_RATE_115200);
	uart_init(BIT_RATE_115200, BIT_RATE_115200);

	system_update_cpu_freq(160);

	uart0_sendStr("\r\n\033c" ); //Clear screen
	uart0_sendStr("\r\n\033c" ); //Clear screen
	uart0_sendStr("\r\nesp82XX Web-GUI\r\n" VERSSTR "\b");

	CSSettingsLoad( 0 );
    CSPreInit();

	CSInit();


	//Set GPIO16 for INput
	WRITE_PERI_REG(PAD_XPD_DCDC_CONF,
		(READ_PERI_REG(PAD_XPD_DCDC_CONF) & 0xffffffbc) | (uint32)0x1);     // mux configuration for XPD_DCDC and rtc_gpio0 connection

	WRITE_PERI_REG(RTC_GPIO_CONF,
		(READ_PERI_REG(RTC_GPIO_CONF) & (uint32)0xfffffffe) | (uint32)0x0); //mux configuration for out enable

	WRITE_PERI_REG(RTC_GPIO_ENABLE,
		READ_PERI_REG(RTC_GPIO_ENABLE) & (uint32)0xfffffffe);       //out disable

	SetServiceName( "ws2812" );
	AddMDNSName( "esp82xx" );
	AddMDNSName( "rawpack" );
	AddMDNSService( "_http._tcp", "An ESP8266 Webserver", WEB_PORT );
	AddMDNSService( "_esp82xx._udp", "ESP8266 Backend", BACKEND_PORT );


    pUdpServer = (struct espconn *)os_zalloc(sizeof(struct espconn));
	ets_memset( pUdpServer, 0, sizeof( struct espconn ) );
	espconn_create( pUdpServer );
	pUdpServer->type = ESPCONN_UDP;
	pUdpServer->proto.udp = (esp_udp *)os_zalloc(sizeof(esp_udp));
	pUdpServer->proto.udp->local_port = 9999;
	pUdpServer->proto.udp->remote_port = 9999;
	pUdpServer->proto.udp->remote_ip[0] = 255;
	pUdpServer->proto.udp->remote_ip[1] = 255;
	pUdpServer->proto.udp->remote_ip[2] = 255;
	pUdpServer->proto.udp->remote_ip[3] = 255;
	espconn_regist_recvcb(pUdpServer, udpserver_recv);
	if( espconn_create( pUdpServer ) )
		while(1)
            uart0_sendStr( "\r\nFAULT\r\n" );


	//XXX TODO figure out how to safely re-allow this.
	ets_wdt_disable();

	//Add a process
	system_os_task(procTask, procTaskPrio, procTaskQueue, procTaskQueueLen);

	uart0_sendStr("\r\nCustom Server\r\n");

	//Timer example
	os_timer_disarm(&some_timer);
	os_timer_setfn(&some_timer, (os_timer_func_t *)myTimer, NULL);
	os_timer_arm(&some_timer, SLOWTICK_MS, 1);
 
	system_os_post(procTaskPrio, 0, 0 );
}

//There is no code in this project that will cause reboots if interrupts are disabled.
void EnterCritical() {}
void ExitCritical() {}
