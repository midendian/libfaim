
/*
 * For working with TIS proxies.  TODO: Fix for use with aim_conn.c.
 *
 */


#include "aim.h"
#ifdef TIS_TELNET_PROXY
#include "tis_telnet_proxy.h"

void
tis_telnet_proxy_connect( int fd, char *host, int port )
{
   fd_set          inset;
   fd_set          outset;
	struct timeval  tv;
	char            connectstring[512];
	char            responsestring[512];
	char           *ptr;
	struct hostent *hent;

	hent = gethostbyname( host );

	snprintf( connectstring, 512, "connect %s %d\n", hent->h_name, port );
	snprintf( responsestring, 512, "Connected to %s.\r\n", hent->h_name);

	FD_ZERO( &outset );
	FD_SET( fd, &outset );
	tv.tv_sec = 0;
	tv.tv_usec = 5;

	if( select( fd + 1, NULL, &outset, NULL, &tv ) == 1 )
		if( write( fd, connectstring, strlen(connectstring) ) !=
			 strlen(connectstring) )
			printf("\n****ERROR ON WRITE**** (proxy connect)\n");


	FD_ZERO( &inset );
	FD_SET( fd, &inset );

	ptr = responsestring;
	while( select( fd + 1, &inset, NULL, NULL, NULL ) == 1 )
	{
		char c;

		if( read( fd, &c, sizeof(c) ) != 1 )
			printf("\n****ERROR ON READ**** (proxy response)\n");

		if( c == *ptr )
		{
			if( *(++ptr) == '\0' )
				break;
		}
		else
			ptr = responsestring;

		FD_ZERO( &inset );
		FD_SET( fd, &inset );
	}
}

#endif /* TIS_TELNET_PROXY */

