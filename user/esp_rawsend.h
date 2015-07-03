#ifndef _ESP_RAWSEND_H
#define _ESP_RAWSEND_H
#include "c_types.h"

void SetupRawsend();
int  CanRawsend();
void RawSendBuffer( uint8_t * buffer, int length );

#endif


