#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <arpa/inet.h>
#include <sys/socket.h>

#define PORT 9999   //The port on which to listen for incoming data
 
void die(char *s)
{
    perror(s);
    exit(1);
}
 
int main(void)
{
    struct sockaddr_in si_me, si_other;
     
    int s, i, slen = sizeof(si_other) , recv_len;
     
    //create a UDP socket
    if ((s=socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP)) == -1)
    {
        die("socket");
    }
     
    // zero out the structure
    memset((char *) &si_me, 0, sizeof(si_me));
     
    si_me.sin_family = AF_INET;
    si_me.sin_port = htons(PORT);
    si_me.sin_addr.s_addr = htonl(INADDR_ANY);
     
    //bind socket to port
    if( bind(s , (struct sockaddr*)&si_me, sizeof(si_me) ) == -1)
    {
        die("bind");
    }

    while(1)
    {
	    uint8_t buf[1024];
        //try to receive some data, this is a blocking call
        if ((recv_len = recvfrom(s, buf, sizeof(buf), 0, (struct sockaddr *) &si_other, &slen)) == -1)
        {
            die("recvfrom()");
        }

		int ipl = 0;
		while( ipl < recv_len )
		{
			uint8_t * bptr = &buf[ipl];
			ipl += 40;

			//printf( "%d ", recv_len );
			printf("%d.%d.%d.%d ",
			  (si_other.sin_addr.s_addr&0xFF),
			  ((si_other.sin_addr.s_addr&0xFF00)>>8),
			  ((si_other.sin_addr.s_addr&0xFF0000)>>16),
			  ((si_other.sin_addr.s_addr&0xFF000000)>>24));

			int i, j, p = 0;
			for( j = 0; j < 3; j++ )
			{
				for( i = 0; i < 6; i++ )
				{
					printf( "%02x", bptr[p++] );
				}
				printf(" ");
			}
			for( i = 0; i < 6; i++ )
			{
				printf( "%c", bptr[p++] );
			}
			uint32_t id = ntohl( *((uint32_t*)&bptr[p]) ); p+=4;
			uint32_t time = ( *((uint32_t*)&bptr[p]) ); p+=4;
			printf( " %u %u\n", id, time );
		}
    }
    return 0;
}
