#include "esp_rawsend.h"
#include <mystuff.h>

//From https://github.com/ernacktob/esp8266_wifi_raw
extern void ICACHE_FLASH_ATTR ppEnqueueRxq(void *);
wifi_raw_recv_cb_fn wifi_mcb;
void ICACHE_FLASH_ATTR wifi_set_raw_recv_cb(wifi_raw_recv_cb_fn rx_fn)
{
	wifi_mcb = rx_fn;
}

void aaEnqueueRxq(void * v)
{
	if (wifi_mcb)
		wifi_mcb((struct RxPacket *)( ((void **)v)[4]), v );
	ppEnqueueRxq(v);
}

//I use a less evasive mechanism to send than the other packet thing.

//We need to get our hands on this pointer, but I don't know it's absolute location
//So, we wait for a TX packet callback and steal it from there.
static void * pxpkt = 0;  //This is the pointer to the structure that let's us raw send.

static void txcb(uint8_t *buf, uint16 reason)
{
	pxpkt = buf;
	return;
#ifdef DO_DEEP_TX_DEBUG
	return;
	char ct[16];
	int i = 0;
	ets_sprintf( ct, "<%p %d --> NODE:%p\n", buf, reason, eagle_lwip_getif(1) );
	uart0_sendStr( ct );
	for( i = 0; i <100; i++ )
	{
		ets_sprintf( ct, "%02x ", buf[i] );
		uart0_sendStr( ct );
	}
	uart0_sendStr( "\n" );

	uint8_t * innerbuf = ((uint32_t*)buf)[1];
	ets_sprintf( ct, "\nInner [1]: %p\n", innerbuf );
	uart0_sendStr( ct );

	for( i = 0; i <100; i++ )
	{
		ets_sprintf( ct, "%02x ", innerbuf[i] );
		uart0_sendStr( ct );
	}

	innerbuf = ((uint32_t*)buf)[4];
	ets_sprintf( ct, "\nInner [4]: %p\n", innerbuf );
	uart0_sendStr( ct );
	for( i = 0; i <100; i++ )
	{
		ets_sprintf( ct, "%02x ", innerbuf[i] );
		uart0_sendStr( ct );
	}


	innerbuf = ((uint32_t*)buf)[8];
	ets_sprintf( ct, "\nInner [8]: %p\n", innerbuf ); //nothing else here...
	uart0_sendStr( ct );
	for( i = 0; i <100; i++ )
	{
		ets_sprintf( ct, "%02x ", innerbuf[i] );
		uart0_sendStr( ct );
	}

	uint32_t * inner8next = innerbuf;
	inner8next = inner8next[5];
	ets_sprintf( ct, "\nInner [8][5]: %p\n", inner8next );  //Nothing more here.
	uart0_sendStr( ct );
	for( i = 0; i <100; i++ )
	{
		ets_sprintf( ct, "%02x ", ((uint8_t*)inner8next)[i] );
		uart0_sendStr( ct );
	}
#endif
}


void SetupRawsend()
{
	int i;
	for( i = 0; i <7; i++ )
	{
		ppRegisterTxCallback( txcb, i );
	}
}

int  CanRawsend()
{
	return pxpkt!=0;
}

//I don't use this anymore, so really no need to do it.
//I strongly recommend using esp freedom if possible.
void RawSendBuffer( uint8_t * buffer, int length )
{
	if( !pxpkt )
	{
		printf( "Nosend." );
		return;
	}

	struct Ctrl{
		uint8_t reserved;  //No idea
		uint8_t channel:4; //This is a guess
		uint8_t size_lsb:4;
		uint8_t size_msb;  //This is almost always 192.
		uint8_t code;
	} * control = ((struct Ctrl**)pxpkt)[1];

#ifdef DO_DEEP_TX_DEBUG
	char buffer[100];
	ets_sprintf( buffer, "Res: %d Chan: %d Size: %d Code: %d // TXLen: %d\n", control->reserved, control->channel, control->size_lsb + (control->size_msb<<4), control->code, ((uint32_t*)pxpkt)[5]>>16 );
	uart0_sendStr(buffer);
#endif

	int size = length;  //Actual size of payload.
	if( size < 24 ) size = 24;

	control->size_lsb = size&0x0f;
	control->size_msb = size>>4;
	((uint32_t*)pxpkt)[5] = (((uint32_t*)pxpkt)[5] & 0xff ) | ((size-24)<<16);

	uint8_t * rawpacket = ((uint8_t**)pxpkt)[4];
	ets_memcpy( rawpacket, buffer, length );
	ppTxPkt( pxpkt );

}


