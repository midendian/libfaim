/*
 *
 *
 *
 *
 */

#include "aimdump.h"

#define DEVNAME "eth0"

pcap_t *pd = NULL;

pcap_t *open_pcap(char *device)
{
  pcap_t *pd = NULL;
  char errbuf[PCAP_ERRBUF_SIZE];

  if (device == NULL)
    {
      if ( (device = pcap_lookupdev(errbuf)) == NULL)
	{
	  fprintf(stderr, "pcap_lookup: %s\n", errbuf);
	  return NULL;
	}
    }
  
  if ( (pd = pcap_open_live(device, 1500, 1, 500, errbuf)) == NULL)
    {
      fprintf(stderr, "pcap_open_live: %s\n", errbuf);
      return NULL;
    }
  
  return pd;
}

void showstats(pcap_t *pd)
{
  struct pcap_stat stats;
  
  fflush(stdout);
  printf("\n");
  
  if (pcap_stats(pd, &stats) < 0)
    {
      fprintf(stderr, "cap_stats: %s\n", pcap_geterr(pd));
      return;
    }
  printf("%d packets recieved by filter\n", stats.ps_recv);
  printf("%d packets dropped by kernel\n", stats.ps_drop);

  return;
}

char *next_pcap(pcap_t *pd, struct pcap_pkthdr *hdr)
{
  char *ptr;
  
  while ( (ptr = (char *)pcap_next(pd, hdr)) == NULL)
    ;

  return ptr;
}

/*
 * This covers stage two of the fast-path, after we've determined its 
 * an incoming RPL frame.  
 *
 */
#if 0
void processframe(struct pcap_pkthdr *hdr, char *pkt, char *device)
{
  static struct ether_header *ether = NULL;
#if 0
  u_int8_t cmpaddr[ETH_ALEN] = {0x10, 0x00, 0x5a, 0x3b, 0x0b, 0x72};
  u_int8_t cmpaddr[ETH_ALEN] = {0x03, 0x00, 0x02, 0x00, 0x00, 0x00};
#endif
  if ( (!hdr) || (!pkt) )
    return;

  ether = (struct ether_header *)pkt;
  
  if (hdr->caplen != hdr->len)
    fprintf(stderr, "rpld: caplen/len mismatch\n");


  if ((clientaddr[0] != 0xff) && cmpether(clientaddr, ether->ether_shost))
    {
      int command = 0x0000;
      fprintf(stderr, "rpld: got a %sframe from our client\n", (cmpether(multicastaddr, ether->ether_dhost))?"multicast ":"");
      command = (pkt[19]<<8)+pkt[20];
      fprintf(stderr, "rpld: command: 0x%04x\n", command);
      
#if 0
      if (command == 0x0001) /* FIND */
	send_found(link, device);
      else if (command == 0x0010) /* Send File Request */
	send_file(link, device);
#endif
      
    }
  else if (cmpether(multicastaddr, ether->ether_dhost))
    {
      if (clientaddr[0] != 0xff)
	{
	  /* we're already servicing another client */
	  fprintf(stderr, "rpld: not adding client %s\n", printether(ether->ether_shost));
	  return;
	}
      else
	{
	  /* we're not busy yet, lets process this one */
	  fprintf(stderr, "rpld: adding client %s\n", printether(ether->ether_shost));
	  memcpy(clientaddr, ether->ether_shost, ETH_ALEN);
	}
    }
#if 0
  else if (cmpether(clientaddr, ether->ether_dhost))
    {
      fprintf(stderr, "rpld: showing outgoing frame...\n");
      printframe(hdr, pkt);
    }
#endif
  else
    fprintf(stderr, "rpld: uncaught case\n");

#if 0
  if (cmpether(cmpaddr, ether->ether_shost) ||
      cmpether(cmpaddr, ether->ether_dhost) )
    {
      printf("\n\n%s > ", printether(ether->ether_shost));
      printf("%s\n", printether(ether->ether_dhost));

      if (pkt[20] != 0x01)
	printf("\aSTATUS CHANGE\n");

      //pkt = 18; /* skip over ethernet headers */
      for (i=0;i<hdr->caplen;i+=2)
	{
	  if (!((i)%8))
	    printf("\n\t");
	  printf("%02x%02x ", pkt[i] &0xff, pkt[i+1] & 0xff);
	}
    }
  else 
    return;
#endif
}
#endif

void sigint(int sig)
{
  fflush(stdout);
  showstats(pd);
  exit(0);
  return;
}

struct flaphdr {
  unsigned char start;
  unsigned char channel;
  unsigned short seqnum;
  unsigned short len;
};

struct snachdr {
  u_short family;
  u_short subtype;
  u_char flags[2];
  u_long id;
};

struct subtype_s {
  char *name;
  void (*parser)(u_char *data, int len);
};

struct snactype_s {
  char *family;
  struct subtype_s subtypes[21];
} snactypes[16] = {
  {"Invalid", {
    {NULL, NULL}}
  },
  {"General", {
    {"Invalid", NULL},
    {"Error", NULL},
    {"Client Ready", NULL},
    {"Server Ready", NULL}, 
    {"Service Request", NULL},
    {"Redirect", NULL},
    {"Rate Information Request", NULL},
    {"Rate Information", NULL},
    {"Rate Information Ack", NULL},
    {"Rate Information Change", NULL},
    {"Server Pause", NULL},
    {"Server Resume", NULL},
    {"Request Personal User Information", NULL},
    {"Personal User Information", NULL},
    {"Evil Notification", NULL},
    {"Migration notice", NULL},
    {"Message of the Day", NULL},
    {"Set Privacy Flags", NULL},
    {"Well Known URL", NULL},
    {"NOP", NULL},
    {NULL, NULL}}
  },
  {"Location", {
    {"Invalid", NULL},
    {"Error", NULL},
    {"Request Rights", NULL},
    {"Rights Information", NULL}, 
    {"Set user information", NULL}, 
    {"Request User Information", NULL}, 
    {"User Information", NULL}, 
    {"Watcher Sub Request", NULL},
    {"Watcher Notification", NULL},
    {NULL, NULL}}
  },
  {"Buddy List Management", {
    {"Invalid", NULL}, 
    {"Error", NULL}, 
    {"Request Rights", NULL},
    {"Rights Information", NULL},
    {"Add Buddy", NULL}, 
    {"Remove Buddy", NULL}, 
    {"Watcher List Query", NULL}, 
    {"Watcher List Response", NULL}, 
    {"Watcher SubRequest", NULL}, 
    {"Watcher Notification", NULL}, 
    {"Reject Notification", NULL}, 
    {"Oncoming Buddy", NULL}, 
    {"Offgoing Buddy", NULL},
    {NULL, NULL}},
  },
  {"Messeging", {
    {"Invalid", NULL},
    {"Error", NULL}, 
    {"Add ICBM Parameter", NULL},
    {"Remove ICBM Parameter", NULL}, 
    {"Request Parameter Information", NULL},
    {"Parameter Information", NULL},
    {"Outgoing Message", NULL}, 
    {"Incoming Message", parser_icbm_incoming},
    {"Evil Request", NULL},
    {"Evil Reply", NULL}, 
    {"Missed Calls", NULL},
    {"Message Error", NULL}, 
    {"Host Ack", NULL},
    {NULL, NULL}}
  },
  {"Advertisements", {
    {"Invalid", NULL}, 
    {"Error", NULL}, 
    {"Request Ad", NULL},
    {"Ad Data (GIFs)", NULL},
    {NULL, NULL}}
  },
  {"Invitation / Client-to-Client", {
    {"Invalid", NULL},
    {"Error", NULL},
    {"Invite a Friend", NULL},
    {"Invitation Ack", NULL},
    {NULL, NULL}}
  },
  {"Administrative", {
    {"Invalid", NULL},
    {"Error", NULL},
    {"Information Request", NULL},
    {"Information Reply", NULL},
    {"Information Change Request", NULL},
    {"Information Chat Reply", NULL},
    {"Account Confirm Request", NULL},
    {"Account Confirm Reply", NULL},
    {"Account Delete Request", NULL},
    {"Account Delete Reply", NULL},
    {NULL, NULL}}
  },
  {"Popups", {
    {"Invalid", NULL},
    {"Error", NULL},
    {"Display Popup", NULL},
    {NULL, NULL}}
  },
  {"BOS", {
    {"Invalid", NULL},
    {"Error", NULL},
    {"Request Rights", NULL},
    {"Rights Response", NULL},
    {"Set group permission mask", NULL},
    {"Add permission list entries", NULL},
    {"Delete permission list entries", NULL},
    {"Add deny list entries", NULL},
    {"Delete deny list entries", NULL},
    {"Server Error", NULL},
    {NULL, NULL}}
  },
  {"User Lookup", {
    {"Invalid", NULL},
    {"Error", NULL},
    {"Search Request", NULL},
    {"Search Response", NULL},
    {NULL, NULL}}
  },
  {"Stats", {
    {"Invalid", NULL},
    {"Error", NULL},
    {"Set minimum report interval", NULL},
    {"Report Events", NULL},
    {NULL, NULL}}
  },
  {"Translate", {
    {"Invalid", NULL},
    {"Error", NULL},
    {"Translate Request", NULL},
    {"Translate Reply", NULL},
    {NULL, NULL}}
  },
  {"Chat Navigation", {
    {"Invalid", NULL},
    {"Error", NULL},
    {"Request rights", NULL},
    {"Request Exchange Information", NULL},
    {"Request Room Information", NULL},
    {"Request Occupant List", NULL},
    {"Search for Room", NULL},
    {"Create Room", NULL},
    {"Navigation Information", NULL},
    {NULL, NULL}}
  },
  {"Chat", {
    {"Invalid", NULL},
    {"Error", NULL},
    {"Room Information Update", NULL},
    {"Users Joined", NULL},
    {"Users Left", NULL},
    {"Outgoing Message", NULL},
    {"Incoming Message", NULL},
    {"Evil Request", NULL},
    {"Evil Reply", NULL},
    {"Chat Error", NULL},
    {NULL, NULL}}
  },
  {NULL, {
    {NULL, NULL}}
  }
};

void detectaim(struct pcap_pkthdr *hdr, char *pkt)
{
  int i;
  struct ether_header *ether = NULL;
  struct iphdr *ip = NULL;
  struct tcphdr*tcp= NULL;
  struct flaphdr *flap = NULL;
  struct snachdr *snac = NULL;
  char *orig = pkt;
  u_int newlen;
  int maxfamily = 0;
  int maxsubtype = 0;
    
  if ( (!hdr) || (!pkt) )
    return;

  if (hdr->caplen != hdr->len)
    fprintf(stderr, "aimdump: caplen/len mismatch\n");

  newlen = hdr->caplen;

  ether = (struct ether_header *)pkt;
  
#if 0
  printf("\n\naimdump: %s > ", printether(ether->ether_shost));
  printf("%s\n", printether(ether->ether_dhost));
#endif

  pkt += sizeof(struct ether_header); /* skip over ether headers */
  newlen -= sizeof(struct ether_header);

  ip = (struct iphdr *)pkt;
  if (ip->version != 0x4)
    return; /* ditch non IPv4 packets */
  pkt += (ip->ihl)*4;
  newlen -= (ip->ihl)*4;

  tcp = (struct tcphdr *)pkt;
  if(!tcp->psh) /* we only want actual data packets */
    return;
  pkt += sizeof(struct tcphdr);
  newlen -= sizeof(struct tcphdr);

  flap = (struct flaphdr *)pkt;

  if (flap->start != 0x2a)
    return; /* ditch non-FLAP packets */

#if 0
  /* TODO: notify user of new connections (SYN) and closed connections (FIN) */
  printf("\nTCP options: %s %s %s %s %s %s\n\n",
	 tcp->fin?"fin":"",
	 tcp->syn?"syn":"",
	 tcp->rst?"rst":"",
	 tcp->psh?"psh":"",
	 tcp->ack?"ack":"",
	 tcp->urg?"urg":"");
#endif

  flap->seqnum = ntohs(flap->seqnum);
  flap->len = ntohs(flap->len);

  snac = (struct snachdr *)(pkt+6);

  snac->family = ntohs(snac->family);
  snac->subtype= ntohs(snac->subtype);
  snac->id = (htons(snac->id & 0x0000ffff)) + (htons(snac->id >>16)<<16);
  
  printf("\n--------------------\n");
  {
    struct in_addr tmpaddr;
    tmpaddr.s_addr = ip->saddr;
    printf("%s -> ", inet_ntoa(tmpaddr));
    tmpaddr.s_addr = ip->daddr;
    printf("%s\n\n", inet_ntoa(tmpaddr));
  }
  printf("FLAP:\n");
  printf("\tChannel:\t0x%02x\t\tSeqNum:\t0x%04x\n\tLength:\t\t0x%04x\n",
	 flap->channel,
	 flap->seqnum,
	 flap->len);
  printf("SNAC:\n");


  /* for overrun checking... */
  for (maxfamily=1;snactypes[maxfamily].family; maxfamily++)
    ;
  if (snac->family <= maxfamily)
    {
      for (maxsubtype=1;snactypes[snac->family].subtypes[maxsubtype].name; maxsubtype++)
	;
    }
  maxfamily--;
  maxsubtype--;

  printf("\tFamily:\t\t0x%04x (%s)\n", 
	 snac->family,
	 (snac->family > maxfamily)?"Out of Range":snactypes[snac->family].family);
  printf("\tSubtype:\t0x%04x (%s)\n",
	 snac->subtype,
	 (snac->subtype > maxsubtype)?"Out of Range":snactypes[snac->family].subtypes[snac->subtype].name);
  printf("\tFlags:\t\t0x%02x,0x%02x\tID:\t0x%08lx\n", snac->flags[0], snac->flags[1], snac->id);

  /* jump around flap+snac */
  pkt += 16;
  newlen -= 16;

  if (snactypes[snac->family].subtypes[snac->subtype].parser)
    (*(snactypes[snac->family].subtypes[snac->subtype].parser))((u_char *)pkt, newlen);

  printf("\nRAW:\n");

  for (i=0;i<newlen;i+=2)
    {
      if (!((i)%16))
	printf("\n\t");
      printf("%02x%02x ", pkt[i] &0xff, pkt[i+1] & 0xff);
    }
  printf("\n\n");

  fflush(stdout);
  pkt = orig;
}

int main(int argc, char **argv)
{
  struct pcap_pkthdr hdr;
  char *pkt;

  if ((pd = open_pcap(NULL)) == NULL)
    {
      fprintf(stderr, "error in open_pcap\n");
      exit (-1);
    }

  signal(SIGINT, sigint);

#if 0  
  if ((localmac = (u_int8_t *)get_hwaddr(link, DEVNAME, errbuf)) == NULL)
    {
      fprintf(stderr, "rpld: Unable to get local MAC address, using default\n");
      localmac = defsource;
    }

  fprintf(stderr, "rpld: Local MAC Address: %s\n", printether((u_int8_t *)localmac));
#endif

#if 1
  while ((pkt = next_pcap(pd, &hdr)))
    {
      detectaim(&hdr, pkt);
      //printframe(&hdr, pkt);
    }
#endif
#if 0
  while (pkt = next_pcap(pd, &hdr))
    {
      /*
       * Since this is the fast-path (it gets executed for 
       * EVERY frame that gets captured -- this could be HUGE
       * on busy 100mb LANs), we need to determine if this frame
       * needs further processing as fast as possible... 
       */
      etherhdr = (struct ether_header *)pkt;
     
      /* this should cover everything */
      if ( (etherhdr->ether_type == (ETHERTYPE_INCOMING<<8)) ||
	   (etherhdr->ether_type == (ETHERTYPE_FOUND<<8)) ||
	   (etherhdr->ether_type == (ETHERTYPE_DATA<<8)) )
	processframe(&hdr, pkt, link, DEVNAME);
      else 
	fprintf(stderr, "rpld: skipping ethertype %04x\n", etherhdr->ether_type);
    }
#endif
  showstats(pd);

  return 0;
}
