/*
 *  aim_im.c
 *
 *  The routines for sending/receiving Instant Messages.
 *
 */

#include <aim.h>

/*
 * Send an ICBM (instant message).  
 *
 *
 * Possible flags:
 *   AIM_IMFLAGS_AWAY  -- Marks the message as an autoresponse
 *   AIM_IMFLAGS_ACK   -- Requests that the server send an ack
 *                        when the message is received (of type 0x0004/0x000c)
 *
 */
u_long aim_send_im(struct aim_conn_t *conn, char *destsn, u_int flags, char *msg)
{   

  int curbyte,i;
  struct command_tx_struct newpacket;
  
  newpacket.lock = 1; /* lock struct */
  newpacket.type = 0x02; /* IMs are always family 0x02 */
  if (conn)
    newpacket.conn = conn;
  else
    newpacket.conn = aim_getconn_type(AIM_CONN_TYPE_BOS);

  /*
   * Its simplest to set this arbitrarily large and waste
   * space.  Precalculating is costly here.
   */
  newpacket.commandlen = 1152;

  newpacket.data = (char *) calloc(1, newpacket.commandlen);

  curbyte  = 0;
  curbyte += aim_putsnac(newpacket.data+curbyte, 
			 0x0004, 0x0006, 0x0000, aim_snac_nextid);

  /* 
   * Generate a random message cookie 
   */
  for (i=0;i<8;i++)
    curbyte += aimutil_put8(newpacket.data+curbyte, (u_char) random());

  /*
   * Channel ID
   */
  curbyte += aimutil_put16(newpacket.data+curbyte,0x0001);

  /* 
   * Destination SN (prepended with byte length)
   */
  curbyte += aimutil_put8(newpacket.data+curbyte,strlen(destsn));
  curbyte += aimutil_putstr(newpacket.data+curbyte, destsn, strlen(destsn));

  /*
   * metaTLV start.
   */
  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0002);
  curbyte += aimutil_put16(newpacket.data+curbyte, strlen(msg) + 0x0d);

  /*
   * Flag data?
   */
  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0501);
  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0001);
  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0101);
  curbyte += aimutil_put8 (newpacket.data+curbyte, 0x01);

  /* 
   * Message block length.
   */
  curbyte += aimutil_put16(newpacket.data+curbyte, strlen(msg) + 0x04);

  /*
   * Character set data? 
   */
  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0000);
  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0000);

  /*
   * Message.  Not terminated.
   */
  curbyte += aimutil_putstr(newpacket.data+curbyte,msg, strlen(msg));

  /*
   * Set the Request Acknowledge flag.  
   */
  if (flags & AIM_IMFLAGS_ACK)
    {
      curbyte += aimutil_put16(newpacket.data+curbyte,0x0003);
      curbyte += aimutil_put16(newpacket.data+curbyte,0x0000);
    }
  
  /*
   * Set the Autoresponse flag.
   */
  if (flags & AIM_IMFLAGS_AWAY)
    {
      curbyte += aimutil_put16(newpacket.data+curbyte,0x0004);
      curbyte += aimutil_put16(newpacket.data+curbyte,0x0000);
    }
  
  newpacket.commandlen = curbyte;

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

/*
 * It can easily be said that parsing ICBMs is THE single
 * most difficult thing to do in the in AIM protocol.  In
 * fact, I think I just did say that.
 *
 * Below is the best damned solution I've come up with
 * over the past sixteen months of battling with it. This
 * can parse both away and normal messages from every client
 * I have access to.  Its not fast, its not clean.  But it works.
 *
 * We should also support at least minimal parsing of 
 * Channel 2, so that we can at least know the name of the
 * room we're invited to, but obviously can't attend...
 *
 */
int aim_parse_incoming_im_middle(struct command_rx_struct *command)
{
  struct aim_userinfo_s userinfo;
  u_int i = 0, j = 0, y = 0, z = 0;
  char *msg = NULL;
  u_int icbmflags = 0;
  rxcallback_t userfunc = NULL;
  u_char cookie[8];
  int channel;
  struct aim_tlvlist_t *tlvlist;
  struct aim_tlv_t *msgblocktlv, *tmptlv;
  u_char *msgblock;
  u_short wastebits;
  u_short flag1,flag2;

  memset(&userinfo, 0x00, sizeof(struct aim_userinfo_s));
  
  i = 10; /* Skip SNAC header */

  /*
   * Read ICBM Cookie.  And throw away.
   */
  for (z=0; z<8; z++,i++)
    cookie[z] = command->data[i];
  
  /*
   * Channel ID.
   *
   * Channel 0x0001 is the message channel.  There are 
   * other channels for things called "rendevous"
   * which represent chat and some of the other new
   * features of AIM2/3/3.5.  We only support 
   * standard messages; those on channel 0x0001.
   */
  channel = aimutil_get16(command->data+i);
  i += 2;
  if (channel != 0x0001)
    {
      printf("faim: icbm: ICBM received on an unsupported channel.  Ignoring.\n (chan = %04x)", channel);
      return 1;
    }

  /*
   * Source screen name.
   */
  memcpy(userinfo.sn, command->data+i+1, (int)command->data[i]);
  userinfo.sn[(int)command->data[i]] = '\0';
  i += 1 + (int)command->data[i];

  /*
   * Unknown bits.
   */
  wastebits = aimutil_get16(command->data+i);
  i += 2;
  wastebits = aimutil_get16(command->data+i);
  i += 2;

  /*
   * Read block of TLVs.  All further data is derived
   * from what is parsed here.
   */
  tlvlist = aim_readtlvchain(command->data+i, command->commandlen-i);

  /*
   * Check Autoresponse status.  If it is an autoresponse,
   * it will contain a second type 0x0004 TLV, with zero length.
   */
  if (aim_gettlv(tlvlist, 0x0004, 2))
    icbmflags |= AIM_IMFLAGS_AWAY;

  /*
   * Check Ack Request status.
   */
  if (aim_gettlv(tlvlist, 0x0003, 2))
    icbmflags |= AIM_IMFLAGS_ACK;

  /*
   * Extract the various pieces of the userinfo struct.
   */
  /* Class. */
  if ((tmptlv = aim_gettlv(tlvlist, 0x0001, 1)))
    userinfo.class = aimutil_get16(tmptlv->value);
  /* Member-since date. */
  if ((tmptlv = aim_gettlv(tlvlist, 0x0002, 1)))
    {
      /* If this is larger than 4, its probably the message block, skip */
      if (tmptlv->length <= 4)
	userinfo.membersince = aimutil_get32(tmptlv->value);
    }
  /* On-since date */
  if ((tmptlv = aim_gettlv(tlvlist, 0x0003, 1)))
    userinfo.onlinesince = aimutil_get32(tmptlv->value);
  /* Idle-time */
  if ((tmptlv = aim_gettlv(tlvlist, 0x0004, 1)))
    userinfo.idletime = aimutil_get16(tmptlv->value);
  /* Session Length (AIM) */
  if ((tmptlv = aim_gettlv(tlvlist, 0x000f, 1)))
    userinfo.sessionlen = aimutil_get16(tmptlv->value);
  /* Session Length (AOL) */
  if ((tmptlv = aim_gettlv(tlvlist, 0x0010, 1)))
    userinfo.sessionlen = aimutil_get16(tmptlv->value);

  /*
   * Message block.
   *
   * XXX: Will the msgblock always be the second 0x0002? 
   */
  msgblocktlv = aim_gettlv(tlvlist, 0x0002, 1);
  if (!msgblocktlv)
    {
      printf("faim: icbm: major error! no message block TLV found!\n");
      aim_freetlvchain(&tlvlist);
    }

  /*
   * Extracting the message from the unknown cruft.
   * 
   * This is a bit messy, and I'm not really qualified,
   * even as the author, to comment on it.  At least
   * its not as bad as a while loop shooting into infinity.
   *
   * "Do you believe in magic?"
   *
   */
  msgblock = msgblocktlv->value;
  j = 0;

  wastebits = aimutil_get8(msgblock+j++);
  wastebits = aimutil_get8(msgblock+j++);
  
  y = aimutil_get16(msgblock+j);
  j += 2;
  for (z = 0; z < y; z++)
    wastebits = aimutil_get8(msgblock+j++);
  wastebits = aimutil_get8(msgblock+j++);
  wastebits = aimutil_get8(msgblock+j++);
 
  /* 
   * Message string length, including flag words.
   */
  i = aimutil_get16(msgblock+j);
  j += 2;

  /*
   * Flag words.
   *
   * Its rumored that these can kick in some funky
   * 16bit-wide char stuff that used to really kill
   * libfaim.  Hopefully the latter is no longer true.
   *
   * Though someone should investiagte the former.
   *
   */
  flag1 = aimutil_get16(msgblock+j);
  j += 2;
  flag2 = aimutil_get16(msgblock+j);
  j += 2;

  if (flag1 || flag2)
    printf("faim: icbm: **warning: encoding flags are being used! {%04x, %04x}\n", flag1, flag2);

  /* 
   * Message string. 
   */
  i -= 4;
  msg = (char *)malloc(i+1);
  memcpy(msg, msgblock+j, i);
  msg[i] = '\0';

  /*
   * Free up the TLV chain.
   */
  aim_freetlvchain(&tlvlist);

  /*
   * Call client.
   */
  userfunc = aim_callhandler(command->conn, 0x0004, 0x0007);
  if (userfunc)
    i = userfunc(command, &userinfo, msg, icbmflags, flag1, flag2);
  else 
    i = 0;

  free(msg);

  return 1;
}

/*
 * Not real sure what this does, nor does anyone I've talk to.
 *
 * Didn't use to send it.  But now I think it might be a good
 * idea. 
 *
 */
u_long aim_seticbmparam(struct aim_conn_t *conn)
{
  struct command_tx_struct newpacket;
  int curbyte;

  newpacket.lock = 1;
  if (conn)
    newpacket.conn = conn;
  else
    newpacket.conn = aim_getconn_type(AIM_CONN_TYPE_BOS);
  newpacket.type = 0x02;

  newpacket.commandlen = 10 + 16;
  newpacket.data = (u_char *) malloc (newpacket.commandlen);

  curbyte = aim_putsnac(newpacket.data, 0x0004, 0x0002, 0x0000, aim_snac_nextid);
  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0000);
  curbyte += aimutil_put32(newpacket.data+curbyte, 0x00000003);
  curbyte += aimutil_put8(newpacket.data+curbyte,  0x1f);
  curbyte += aimutil_put8(newpacket.data+curbyte,  0x40);
  curbyte += aimutil_put8(newpacket.data+curbyte,  0x03);
  curbyte += aimutil_put8(newpacket.data+curbyte,  0xe7);
  curbyte += aimutil_put8(newpacket.data+curbyte,  0x03);
  curbyte += aimutil_put8(newpacket.data+curbyte,  0xe7);
  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0000);
  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0000);

  aim_tx_enqueue(&newpacket);

  return (aim_snac_nextid++);
}
