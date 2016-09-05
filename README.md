#Demonstration of raw packet sending on the ESP8266

BIG BIG NOTES: This no longer works as you'd expect, also the tx packets seem to be broken, and for rx I copied this guy's stuff : https://github.com/ernacktob/esp8266_wifi_raw

Big thing I'm trying to do is turn these into a mechanism to watch timing between packets.  This is more a dumping ground, DO NOT USE IT AS REGULAR PROJECT.

This actually works, I was digging around ppTxPkt and found that the structure sent back from the ppRegisterTxCallback seems to match that of what ppTxPkt expects. 

Oddly enough it appears all the buffers get set up deep in the bowels of libpp.a(esf_buf.o).  So, I figure I can just re-use those buffers, but change what we need to change and sure enough it worked!  Check out esp_rawsend.c

NOW, if only someone could figure out how to do promiscuous mode simultaneous to linking to linking to networks and normal operation we'd have something really awesome. 
