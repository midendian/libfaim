#ifndef __AIMDUMP_H__
#define __AIMDUMP_H__

#include <unistd.h>
#include <netinet/if_ether.h>
#include <pcap/pcap.h>
#include "util.h"
#include <signal.h>
#include <netinet/ip.h>
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <arpa/inet.h>

/*
 * These are in NETWORK (big-endian) BYTE ORDER!!!
 */
#define SAPLEN 3 /* the length of the SAP header */
#define SAPHEADER {0xfc, 0xfc, 0x03} /* SAP header bytes */
#define ETHERTYPE_INCOMING 0x0056 /* on data from the 8227 */
#define ETHERTYPE_FOUND 0x003d /* on outgoing FOUND frames */
#define ETHERTYPE_DATA 0x05dc /* on outgoing Data frames */

void parser_icbm_incoming(u_char *data, int len);

#endif /* __AIMDUMP_H__ */
