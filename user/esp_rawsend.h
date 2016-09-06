#ifndef _ESP_RAWSEND_H
#define _ESP_RAWSEND_H
#include "c_types.h"



//from https://github.com/ernacktob/esp8266_wifi_raw


struct RxControl {
	signed rssi:8;
	unsigned rate:4;
	unsigned is_group:1;
	unsigned:1;
	unsigned sig_mode:2;
	unsigned legacy_length:12;
	unsigned damatch0:1;
	unsigned damatch1:1;
	unsigned bssidmatch0:1;
	unsigned bssidmatch1:1;
	unsigned MCS:7;
	unsigned CWB:1;
	unsigned HT_length:16;
	unsigned Smoothing:1;
	unsigned Not_Sounding:1;
	unsigned:1;
	unsigned Aggregation:1;
	unsigned STBC:2;
	unsigned FEC_CODING:1;
	unsigned SGI:1;
	unsigned rxend_state:8;
	unsigned ampdu_cnt:8;
	unsigned channel:4;
	unsigned:12;
};

struct RxPacket {
	struct RxControl rx_ctl;
	uint8 data[];
};

typedef void (*wifi_raw_recv_cb_fn)(struct RxPacket *, void ** v);
void ICACHE_FLASH_ATTR wifi_set_raw_recv_cb(wifi_raw_recv_cb_fn rx_fn);

//My stuff here.
//Don't use this stuff.
void SetupRawsend();
int  CanRawsend();
void RawSendBuffer( uint8_t * buffer, int length );

#endif


