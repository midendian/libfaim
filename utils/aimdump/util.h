/*
 *
 */
#ifndef __UTIL_H__
#define __UTIL_H__

char *printether(u_int8_t *addr);
int cmpether(u_int8_t *addr, u_int8_t *addr2);
void printframe(struct pcap_pkthdr *hdr, char *pkt);

#endif /* __UTIL_H__ */ 
