/*
 *  aim_im.c
 *
 *  The routines for sending/receiving Instant Messages.
 *
 */

#include "aim.h"

/*
 * Send an ICBM (instant message).  
 *
 *
 * Possible flags:
 *   AIM_IMFLAGS_AWAY  -- Marks the message as an autoresponse
 *   AIM_IMFLAGS_ACK   -- Requests that the server send an ack
 *                        when the message is received (of type 0x0004/0x000c)
 *
 *
 * The first one is newer, but it seems to be broken.  Beware.  Use the 
 * second one for normal use...for now.
 *
 *
 */

#if 0 /* preliminary new fangled routine */
u_long aim_send_im(struct aim_conn_t *conn, char *destsn, int flags, char *msg)
{   

  int curbyte;
  struct command_tx_struct newpacket;
  
  newpacket.lock = 1; /* lock struct */
  newpacket.type = 0x02; /* IMs are always family 0x02 */
  if (conn)
    newpacket.conn = conn;
  else
    newpacket.conn = aim_getconn_type(AIM_CONN_TYPE_BOS);

  newpacket.commandlen = 20+1+strlen(destsn)+1+1+2+7+2+4+strlen(msg)+2;

  if (flags & AIM_IMFLAGS_ACK)
    newpacket.commandlen += 4;
  if (flags & AIM_IMFLAGS_AWAY)
    newpacket.commandlen += 4;

  newpacket.data = (char *) calloc(1, newpacket.commandlen);

  curbyte  = 0;
  curbyte += aim_putsnac(newpacket.data+curbyte, 0x0004, 0x0006, 0x0000, aim_snac_nextid);
  curbyte += aimutil_put16(newpacket.data+curbyte,0x0000);
  curbyte += aimutil_put16(newpacket.data+curbyte,0x0000);
  curbyte += aimutil_put16(newpacket.data+curbyte,0x0000);
  curbyte += aimutil_put16(newpacket.data+curbyte,0x0000);
  curbyte += aimutil_put16(newpacket.data+curbyte,0x0001);
  curbyte += aimutil_put8(newpacket.data+curbyte,strlen(destsn));
  curbyte += aimutil_putstr(newpacket.data+curbyte, destsn, strlen(destsn));

  if (flags & AIM_IMFLAGS_ACK)
    {
      curbyte += aimutil_put16(newpacket.data+curbyte,0x0003);
      curbyte += aimutil_put16(newpacket.data+curbyte,0x0000);
    }

  if (flags & AIM_IMFLAGS_AWAY)
    {
      curbyte += aimutil_put16(newpacket.data+curbyte,0x0004);
      curbyte += aimutil_put16(newpacket.data+curbyte,0x0000);
    }

  curbyte += aimutil_put16(newpacket.data+curbyte,0x0002);
  curbyte += aimutil_put16(newpacket.data+curbyte,strlen(msg)+0xf);
  curbyte += aimutil_put16(newpacket.data+curbyte,0x0501);
  curbyte += aimutil_put16(newpacket.data+curbyte,0x0001);
  curbyte += aimutil_put16(newpacket.data+curbyte,0x0101);
  curbyte += aimutil_put16(newpacket.data+curbyte,0x0101);
  curbyte += aimutil_put8(newpacket.data+curbyte,0x01);
  curbyte += aimutil_put16(newpacket.data+curbyte,strlen(msg)+4);
  curbyte += aimutil_put16(newpacket.data+curbyte,0x0000);
  curbyte += aimutil_put16(newpacket.data+curbyte,0x0000);
  curbyte += aimutil_putstr(newpacket.data+curbyte, msg, strlen(msg));

  aim_tx_enqueue(&newpacket);

#ifdef USE_SNAC_FOR_IMS
 {
    struct aim_snac_t snac;

    snac.id = aim_snac_nextid;
    snac.family = 0x0004;
    snac.type = 0x0006;
    snac.flags = 0x0000;

    snac.data = malloc(strlen(destsn)+1);
    memcpy(snac.data, destsn, strlen(destsn)+1);

    aim_newsnac(&snac);
  }

 aim_cleansnacs(60); /* clean out all SNACs over 60sec old */
#endif

  return (aim_snac_nextid++);
}
#else
u_long aim_send_im(struct aim_conn_t *conn, char *destsn, int flags, char *msg)
{

  int i;
  struct command_tx_struct newpacket;

  newpacket.lock = 1; /* lock struct */
  newpacket.type = 0x02; /* IMs are always family 0x02 */
  if (conn)
    newpacket.conn = conn;
  else
    newpacket.conn = aim_getconn_type(AIM_CONN_TYPE_BOS);

  i = strlen(destsn); /* used as offset later */
  newpacket.commandlen = 20+1+strlen(destsn)+1+1+2+7+2+4+strlen(msg);

  if (flags & AIM_IMFLAGS_ACK)
    newpacket.commandlen += 4;
  if (flags & AIM_IMFLAGS_AWAY)
    newpacket.commandlen += 4;

  newpacket.data = (char *) malloc(newpacket.commandlen);
  memset(newpacket.data, 0x00, newpacket.commandlen);

  aim_putsnac(newpacket.data, 0x0004, 0x0006, 0x0000, aim_snac_nextid);

  newpacket.data[18] = 0x00;
  newpacket.data[19] = 0x01;

  newpacket.data[20] = i;
  memcpy(&(newpacket.data[21]), destsn, i);

  if (flags & AIM_IMFLAGS_ACK)
    {
      /* add TLV t(0004) l(0000) v(NULL) */
      newpacket.data[21+i] = 0x00;
      newpacket.data[22+i] = 0x03;
      newpacket.data[23+i] = 0x00;
      newpacket.data[24+i] = 0x00;
      i += 4;
    }
  if (flags & AIM_IMFLAGS_AWAY)
    {
      /* add TLV t(0004) l(0000) v(NULL) */
      newpacket.data[21+i] = 0x00;
      newpacket.data[22+i] = 0x04;
      newpacket.data[23+i] = 0x00;
      newpacket.data[24+i] = 0x00;
      i += 4;
    }

  newpacket.data[21+i] = 0x00;

  newpacket.data[22+i] = 0x02;

  newpacket.data[23+i] = (char) ( (strlen(msg) + 0xD) >> 8);
  newpacket.data[24+i] = (char) ( (strlen(msg) + 0xD) & 0xFF);

  newpacket.data[25+i] = 0x05;
  newpacket.data[26+i] = 0x01;
  newpacket.data[27+i] = 0x00;
  newpacket.data[28+i] = 0x01;
  newpacket.data[29+i] = 0x01;
  newpacket.data[30+i] = 0x01;
  newpacket.data[31+i] = 0x01;

  newpacket.data[32+i] = (char) ( (strlen(msg) + 4) >> 8);
  newpacket.data[33+i] = (char) ( (strlen(msg) + 4) & 0xFF);

  memcpy(&(newpacket.data[38+i]), msg, strlen(msg));

  aim_tx_enqueue(&newpacket);
#ifdef USE_SNAC_FOR_IMS
 {
    struct aim_snac_t snac;

    snac.id = aim_snac_nextid;
    snac.family = 0x0004;
    snac.type = 0x0006;
    snac.flags = 0x0000;

    snac.data = malloc(strlen(destsn)+1);
    memcpy(snac.data, destsn, strlen(destsn)+1);

    aim_newsnac(&snac);
  }

 aim_cleansnacs(60); /* clean out all SNACs over 60sec old */
#endif

  return (aim_snac_nextid++);
}

#endif

#if 0 /* this is the prelim work on a new routine */
int aim_parse_incoming_im_middle(struct command_rx_struct *command)
{
  int i = 0;
  char *srcsn = NULL;
  char *msg = NULL;
  u_int msglen = 0;
  int warninglevel = 0;
  int tlvcnt = 0;
  int class = 0;
  u_long membersince = 0;
  u_long onsince = 0;
  int idletime = 0;
  int isautoreply = 0;
  rxcallback_t userfunc = NULL;

  int unknown_f = -1;
  int unknown_10 = -1;

  i = 20; /* skip SNAC header and message cookie */
  
  srcsn = malloc(command->data[i] + 1);
  memcpy(srcsn, &(command->data[i+1]), command->data[i]);
  srcsn[(int)command->data[i]] = '\0';
  
  i += (int) command->data[i] + 1; /* add SN len */
  
  /* warning level */
  warninglevel = (command->data[i] << 8);
  warninglevel += (command->data[i+1]);
  i += 2;
  
  /*
   * This is suppose to be the number of TLVs that follow.  However,
   * its not nearly as accurate as we need it to be, so we just run
   * the TLV parser all the way to the end of the frame.  
   */
  tlvcnt = ((command->data[i++]) << 8) & 0xFF00;
  tlvcnt += (command->data[i++]) & 0x00FF;
  
  /* a mini TLV parser */
  {
    int curtlv = 0;
    int count_t4 = 0, count_t3 = 0, count_t2 = 0, count_t1 = 0;
    
    while (i+4 < command->commandlen)
      {
	if ((command->data[i] == 0x00) &&
	    (command->data[i+1] == 0x01) )
	  {
	    if (count_t1 == 0)
	      {
		/* t(0001) = class */
		if (command->data[i+3] != 0x02)
		  printf("faim: userinfo: **warning: strange v(%x) for t(1)\n", command->data[i+3]);
		class = ((command->data[i+4]) << 8) & 0xFF00;
		class += (command->data[i+5]) & 0x00FF;
	      }
	    else
	      printf("faim: icbm: unexpected extra TLV t(0001)\n");
	    count_t1++;
	  }
	else if ((command->data[i] == 0x00) &&
		 (command->data[i+1] == 0x02))
	  {
	    if (count_t2 == 0)
	      {
		/* t(0002) = member since date  */
		if (command->data[i+3] != 0x04)
		  printf("faim: userinfo: **warning: strange v(%x) for t(2)\n", command->data[i+3]);
		
		membersince = ((command->data[i+4]) << 24) &  0xFF000000;
		membersince += ((command->data[i+5]) << 16) & 0x00FF0000;
		membersince += ((command->data[i+6]) << 8) &  0x0000FF00;
		membersince += ((command->data[i+7]) ) &      0x000000FF;
	      }
	    else if (count_t2 == 1)
	      {
		int biglen = 0, innerlen = 0;
		int j;

		/* message */

		/* 
		 * Check for message signature (0x0501).  I still don't really
		 * like this, but it is better than the old way, and it does
		 * seem to be consistent as long as AOL doesn't do any more
		 * big changes.
		 */
		if ( (command->data[i+4] != 0x05) ||
		     (command->data[i+5] != 0x01) )
		  printf("faim: icbm: warning: message signature not present, trying to recover\n");
		
		biglen = ((command->data[i+2] << 8) + command->data[i+3]) & 0xffff;

		printf("faim: icbm: biglen = %02x\n", biglen);

		j = 0;
		while (j+3 < (biglen-6))
		  {
		    if ( (command->data[i+6+j+0] == 0x00) &&
			 (command->data[i+6+j+1] == 0x00) &&
			 (command->data[i+6+j+2] == 0x00) )
		      {
			
			innerlen = (command->data[i+6+j]<<8) + command->data[i+6+j+1];
			break;
		      }
		    j++;
		  }
		if (!innerlen)
		  {
		    printf("faim: icbm: unable to find holy zeros; skipping message\n");
		    msglen = 0;
		    msg = NULL;
		  }
		else
		  {
		    printf("faim: icbm: innerlen = %d\n", innerlen);
		    
		    msglen = innerlen - 4;
		    printf("faim: icbm: msglen = %u\n", msglen);
		    
		    msg = malloc(msglen +1);
		    memcpy(msg, &(command->data[i+6+j+4+1]), msglen);
		    msg[msglen] = '\0'; 
		  }
	      }
	    else
	      printf("faim: icbm: **warning: extra TLV t(0002)\n");
	    count_t2++;
	  }
	else if ((command->data[i] == 0x00) &&
		 (command->data[i+1] == 0x03))
	  {
	    if (count_t3 == 0)
	      {
		/* t(0003) = on since date  */
		if (command->data[i+3] != 0x04)
		  printf("faim: userinfo: **warning: strange v(%x) for t(3)\n", command->data[i+3]);
		
		onsince = ((command->data[i+4]) << 24) &  0xFF000000;
		onsince += ((command->data[i+5]) << 16) & 0x00FF0000;
		onsince += ((command->data[i+6]) << 8) &  0x0000FF00;
		onsince += ((command->data[i+7]) ) &      0x000000FF;
	      }
	    else if (count_t3 == 1)
	      printf("faim: icbm: request for acknowledgment ignored\n");
	    else
	      printf("faim: icbm: unexpected extra TLV t(0003)\n");
	    count_t3++;
	  }
	else if ((command->data[i] == 0x00) &&
		 (command->data[i+1] == 0x04) )
	  {
	    if (count_t4 == 0)
	      {
		/* t(0004) = idle time */
		if (command->data[i+3] != 0x02)
		  printf("faim: userinfo: **warning: strange v(%x) for t(4)\n", command->data[i+3]);
		idletime = ((command->data[i+4]) << 8) & 0xFF00;
		idletime += (command->data[i+5]) & 0x00FF;
	      }
	    else if ((count_t4 == 1) && (((command->data[i+2]<<8)+command->data[i+3])==0x0000))
	      isautoreply = 1;
	    else
	      printf("faim: icbm: unexpected extra TLV t(0004)\n");
	    count_t4++;
	  } 
	else if ((command->data[i] == 0x00) &&
		 (command->data[i+1] == 0x0f))
	  {
	    /* t(000f) = unknown...usually from AIM3 users */
	    if (command->data[i+3] != 0x04)
	      printf("faim: userinfo: **warning: strange v(%x) for t(f)\n", command->data[i+3]);
	    unknown_f = (command->data[i+4] << 24) & 0xff000000;
	    unknown_f += (command->data[i+5] << 16) & 0x00ff0000;
	    unknown_f += (command->data[i+6] <<  8) & 0x0000ff00;
	    unknown_f += (command->data[i+7]) & 0x000000ff;
	  }
        else if ((command->data[i] == 0x00) &&
                 (command->data[i+1] == 0x10))
          {
            /* t(0010) = unknown...usually from AOL users */
            if (command->data[i+3] != 0x04)
              printf("faim: userinfo: **warning: strange v(%x) for t(10)\n", command->data[i+3]);
            unknown_10 = (command->data[i+4] << 24) & 0xff000000;
            unknown_10 += (command->data[i+5] << 16) & 0x00ff0000;
            unknown_10 += (command->data[i+6] <<  8) & 0x0000ff00;
            unknown_10 += (command->data[i+7]) & 0x000000ff;
          }
	else
	  {
	    printf("faim: userinfo: **warning: unexpected TLV t(%02x%02x) l(%02x%02x)\n", command->data[i], command->data[i+1], command->data[i+2], command->data[i+3]);
	  }
	i += (2 + 2 + ((command->data[i+2] << 8) + command->data[i+3]));
	curtlv++;
      }
  }

#if 0
  {
    /* detect if this is an auto-response or not */
    /*   auto-responses can be detected by the presence of a *second* TLV with
	 t(0004), but of zero length (and therefore no value portion) */
    struct aim_tlv_t *tsttlv = NULL;
    tsttlv = aim_grabtlv((u_char *) &(command->data[i]));
    if (tsttlv->type == 0x04)
      isautoreply = 1;
    aim_freetlv(&tsttlv);
  }

  i += 2;
  
  i += 2; /* skip first msglen */
  i += 7; /* skip garbage */
  i -= 4;

  /* oh boy is this terrible...  this comes from a specific of the spec */
  while(1)
    {
      if ( ( (command->data[i] == 0x00) &&
	     (command->data[i+1] == 0x00) &&
	     (command->data[i+2] == 0x00) &&
	     (command->data[i+3] == 0x00) ) &&
	   (i < command->commandlen) ) /* prevent infinity */
	break;
      else
	i++;
    }

  i -= 2;
  
  if ( (command->data[i] == 0x00) &&
       (command->data[i+1] == 0x00) )
    i += 2;

  msglen = ( (( (u_int) command->data[i]) & 0xFF ) << 8);
  msglen += ( (u_int) command->data[i+1]) & 0xFF; /* mask off garbage */
  i += 2;

  msglen -= 4; /* skip four 0x00s */
  i += 4;
  
  msg = malloc(msglen +1);
  
  memcpy(msg, &(command->data[i]), msglen);
  msg[msglen] = '\0'; 
#endif
  userfunc = aim_callhandler(command->conn, 0x0004, 0x0007);
  if (userfunc)
    i = userfunc(command, srcsn, msg, warninglevel, class, membersince, onsince, idletime, isautoreply, unknown_f, unknown_10);
  else 
    i = 0;

  free(srcsn);
  free(msg);

  return i;
}
#else /* older routine, with new fixes */
int aim_parse_incoming_im_middle(struct command_rx_struct *command)
{
  struct aim_userinfo_s userinfo;
  u_int i = 0;
  char *msg = NULL;
  u_int msglen = 0;
  int isautoreply = 0;
  rxcallback_t userfunc = NULL;

  i = 20;
  i += aim_extractuserinfo(command->data+i, &userinfo);

  {
    /* 
     *  Auto-responses can be detected by the presence of a *second* TLV with
     *  t(0004), but of zero length (and therefore no value portion) 
     */
    struct aim_tlv_t *tsttlv = NULL;
    tsttlv = aim_grabtlv((u_char *) &(command->data[i]));
    if (tsttlv->type == 0x04)
      isautoreply = 1;
#if 0
    else if (tsttlv->type == 0x03)
      {
	printf("faim: icbm: ack requested, ignored\n");
	i += 2 + 2 + tsttlv->length;
	aim_freetlv(&tsttlv);
	tsttlv = aim_grabtlv((u_char *) &(command->data[i]));
	if (tsttlv->type == 0x04)
	  isautoreply = 1;
      }
#endif
    aim_freetlv(&tsttlv);
  }
  
  i += 2;
  
  i += 2; /* skip first msglen */
  i += 7; /* skip garbage */
  i -= 4;

  /* oh boy is this terrible...  this comes from a specific of the spec */
  while(1)
    {
      /* 
       * We used to look for four zeros; I've reduced this to three
       * as it seems AOL changed it with Mac AIM 3.0 clients.
       */
      if ( ( (command->data[i] == 0x00) &&
	     (command->data[i+1] == 0x00) &&
	     (command->data[i+2] == 0x00) ) &&
	   (i+2 < command->commandlen) ) /* prevent infinity */
	break;
      else
	i++;
    }

  i -= 2;
  
  if (aimutil_get16(&command->data[i]) == 0x0000)
    i += 2;

  msglen = aimutil_get16(&command->data[i]);
  i += 2;

  msglen -= 4; /* skip four 0x00s */
  i += 4;
  
  msg = malloc(msglen +1);
  
  memcpy(msg, &(command->data[i]), msglen);
  msg[msglen] = '\0'; 

  userfunc = aim_callhandler(command->conn, 0x0004, 0x0007);

  if (userfunc)
    i = userfunc(command, &userinfo, msg, isautoreply);
  else 
    i = 0;

  free(msg);

  return i;
}
#endif
