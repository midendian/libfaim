/*
 *
 *
 *
 */

#include "aimdump.h"

char *printether(u_int8_t *addr)
{
  static char out[18];
  
  memset(out, 0, 18);
  sprintf(out, "%02x:%02x:%02x:%02x:%02x:%02x", 
	  addr[0], addr[1],
	  addr[2], addr[3],
	  addr[4], addr[5]);
  
  return out;
}

int cmpether(u_int8_t *addr, u_int8_t *addr2)
{
  int i;
  for (i=0; i<ETH_ALEN; i++)
    {
      if (addr[i] != addr2[i])
	return 0;
    }
  return 1;
}

void printframe(struct pcap_pkthdr *hdr, char *pkt)
{
  int i;
  struct ether_header *ether = NULL;

  if ( (!hdr) || (!pkt) )
    return;

  if (hdr->caplen != hdr->len)
    fprintf(stderr, "rpld: caplen/len mismatch\n");

  ether = (struct ether_header *)pkt;
  
  printf("\n\nrpld: %s > ", printether(ether->ether_shost));
  printf("%s\n", printether(ether->ether_dhost));
  
  //pkt = 18; /* skip over ethernet headers */
  for (i=0;i<hdr->caplen;i+=2)
    {
      if (!((i)%8))
	printf("\nrpld:\t");
      printf("%02x%02x ", pkt[i] &0xff, pkt[i+1] & 0xff);
    }
}
