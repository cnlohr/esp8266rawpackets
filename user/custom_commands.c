//Copyright 2015 <>< Charles Lohr, see LICENSE file.

#include <esp82xxutil.h>
#include <commonservices.h>
extern uint32_t debugccount;
extern uint32_t debugccount2;

int ICACHE_FLASH_ATTR CustomCommand(char * buffer, int retsize, char *pusrdata, unsigned short len)
{
	char * buffend = buffer;

	switch( pusrdata[1] )	{
    	case 'C': case 'c': { //Custom command test
    		buffend += ets_sprintf( buffend, "CC" );
    		return buffend-buffer;
    	} break;

		case 'R': case 'r': {
			ParamCaptureAndAdvanceInt( );
			uint32_t address = ParamCaptureAndAdvanceInt( );
			if( address == 0 )
			{
				buffend += ets_sprintf( buffend, "!CR" );
			}
			else
			{
				buffend += ets_sprintf( buffend, "CR\t%u", *(((uint32_t*)address)) );
			}
			return buffend-buffer;
		}
		case 'W': case 'w': {
			ParamCaptureAndAdvanceInt( );
			uint32_t address = ParamCaptureAndAdvanceInt( );
			uint32_t value = ParamCaptureAndAdvanceInt( );

			if( address == 0 )
			{
				buffend += ets_sprintf( buffend, "!CW" );
			}
			else
			{
				(*((volatile uint32_t*)address)) = value;
				buffend += ets_sprintf( buffend, "Cw" );
			}
			return buffend-buffer;
		}
		case 'd': case 'D': {
			buffend += ets_sprintf( buffend, "CD\t%d\t%d", debugccount, debugccount2 );
			return buffend-buffer;
		}
	}
	return -1;
}
