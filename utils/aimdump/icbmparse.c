

#include <faim/aim.h>

void parser_icbm_incoming(u_char *data, int len)
{
  struct aim_userinfo_s userinfo;
  u_int i = 0, j = 0, y = 0, z = 0;
  char *msg = NULL;
  u_int icbmflags = 0;
  u_char cookie[8];
  int channel;
  struct aim_tlvlist_t *tlvlist;
  struct aim_tlv_t *msgblocktlv, *tmptlv;
  u_char *msgblock;
  u_short wastebits;
  u_short flag1,flag2;

  memset(&userinfo, 0x00, sizeof(struct aim_userinfo_s));

  i = 0;
  /*
   * Read ICBM Cookie.  And throw away.
   */
  for (z=0; z<8; z++,i++)
    cookie[z] = data[i];
  
  /*
   * Channel ID.
   *
   * Channel 0x0001 is the message channel.  There are 
   * other channels for things called "rendevous"
   * which represent chat and some of the other new
   * features of AIM2/3/3.5.  We only support 
   * standard messages; those on channel 0x0001.
   */
  channel = aimutil_get16(data+i);
  i += 2;
  if (channel != 0x0001)
    {
      printf("faim: icbm: ICBM received on an unsupported channel.  Ignoring.\n (chan = %04x)", channel);
      return;
    }

  /*
   * Source screen name.
   */
  memcpy(userinfo.sn, data+i+1, (int)data[i]);
  userinfo.sn[(int)data[i]] = '\0';
  i += 1 + (int)data[i];

  /*
   * Unknown bits.
   */
  wastebits = aimutil_get16(data+i);
  i += 2;
  wastebits = aimutil_get16(data+i);
  i += 2;

  /*
   * Read block of TLVs.  All further data is derived
   * from what is parsed here.
   */
  tlvlist = aim_readtlvchain(data+i, len-i);

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

  printf("ICBM:\n");
  printf("\tChannel:\t0x%04x\n", channel);
  printf("\tSource:\t%s\n", userinfo.sn);
  printf("\tICBM Flags:\t%s %s\n", 
	 (icbmflags & AIM_IMFLAGS_AWAY)?"Away":"", 
	 (icbmflags & AIM_IMFLAGS_ACK)?"Ack":"");
  printf("\tEncoding Flags:\t{0x%02x, 0x%02x}\n", flag1, flag2);
  printf("\tMessage:\n");
  printf("\t\t%s\n", msg);

  free(msg);

  return;
}
