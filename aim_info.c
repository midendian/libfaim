/*
 * aim_info.c
 *
 * The functions here are responsible for requesting and parsing information-
 * gathering SNACs.  
 *
 */


#include <faim/aim.h>

u_long aim_getinfo(struct aim_session_t *sess,
		   struct aim_conn_t *conn, 
		   const char *sn)
{
  struct command_tx_struct newpacket;
  int i = 0;

  if (!sess || !conn || !sn)
    return 0;

  if (conn)
    newpacket.conn = conn;
  else
    newpacket.conn = aim_getconn_type(sess, AIM_CONN_TYPE_BOS);

  newpacket.lock = 1;
  newpacket.type = 0x0002;

  newpacket.commandlen = 12 + 1 + strlen(sn);
  newpacket.data = (char *) malloc(newpacket.commandlen);

  i = aim_putsnac(newpacket.data, 0x0002, 0x0005, 0x0000, sess->snac_nextid);

  i += aimutil_put16(newpacket.data+i, 0x0001);
  i += aimutil_put8(newpacket.data+i, strlen(sn));
  i += aimutil_putstr(newpacket.data+i, sn, strlen(sn));

  newpacket.lock = 0;
  aim_tx_enqueue(sess, &newpacket);

  {
    struct aim_snac_t snac;
    
    snac.id = sess->snac_nextid;
    snac.family = 0x0002;
    snac.type = 0x0005;
    snac.flags = 0x0000;

    snac.data = malloc(strlen(sn)+1);
    strcpy(snac.data, sn);

    aim_newsnac(sess, &snac);
  }

  return (sess->snac_nextid++);
}

/*
 * AIM is fairly regular about providing user info.  This
 * is a generic routine to extract it in its standard form.
 */
int aim_extractuserinfo(u_char *buf, struct aim_userinfo_s *outinfo)
{
  int i = 0;
  int tlvcnt = 0;
  int curtlv = 0;
  int tlv1 = 0;
  u_short curtype;


  if (!buf || !outinfo)
    return -1;

  /* Clear out old data first */
  memset(outinfo, 0x00, sizeof(struct aim_userinfo_s));

  /*
   * Screen name.    Stored as an unterminated string prepended
   *                 with an unsigned byte containing its length.
   */
  memcpy(outinfo->sn, &(buf[i+1]), buf[i]);
  outinfo->sn[(int)buf[i]] = '\0';
  i = 1 + (int)buf[i];

  /*
   * Warning Level.  Stored as an unsigned short.
   */
  outinfo->warnlevel = aimutil_get16(&buf[i]);
  i += 2;

  /*
   * TLV Count.      Unsigned short representing the number of 
   *                 Type-Length-Value triples that follow.
   */
  tlvcnt = aimutil_get16(&buf[i]);
  i += 2;

  /* 
   * Parse out the Type-Length-Value triples as they're found.
   */
  while (curtlv < tlvcnt)
    {
      curtype = aimutil_get16(&buf[i]);
      switch (curtype)
	{
	  /*
	   * Type = 0x0001: Member Class.   
	   * 
	   * Specified as any of the following bitwise ORed together:
	   *      0x0001  Trial (user less than 60days)
	   *      0x0002  Unknown bit 2
	   *      0x0004  AOL Main Service user
	   *      0x0008  Unknown bit 4
	   *      0x0010  Free (AIM) user 
	   *      0x0020  Away
	   *
	   * In some odd cases, we can end up with more
	   * than one of these.  We only want the first,
	   * as the others may not be something we want.
	   *
	   */
	case 0x0001:
	  if (tlv1) /* use only the first */
	    break;
	  outinfo->class = aimutil_get16(&buf[i+4]);
	  tlv1++;
	  break;
	  
	  /*
	   * Type = 0x0002: Member-Since date. 
	   *
	   * The time/date that the user originally
	   * registered for the service, stored in 
	   * time_t format
	   */
	case 0x0002: 
	  outinfo->membersince = aimutil_get32(&buf[i+4]);
	  break;
	  
	  /*
	   * Type = 0x0003: On-Since date.
	   *
	   * The time/date that the user started 
	   * their current session, stored in time_t
	   * format.
	   */
	case 0x0003:
	  outinfo->onlinesince = aimutil_get32(&buf[i+4]);
	  break;

	  /*
	   * Type = 0x0004: Idle time.
	   *
	   * Number of seconds since the user
	   * actively used the service.
	   */
	case 0x0004:
	  outinfo->idletime = aimutil_get16(&buf[i+4]);
	  break;

	  /*
	   * Type = 0x000d
	   *
	   * Capability information.  Not real sure of
	   * actual decoding.  See comment on aim_bos_setprofile()
	   * in aim_misc.c about the capability block, its the same.
	   *
	   * Ignore.
	   *
	   */
	case 0x000d:
	  break;

	  /*
	   * Type = 0x000e
	   *
	   * Unknown.  Always of zero length, and always only
	   * on AOL users.
	   *
	   * Ignore.
	   *
	   */
	case 0x000e:
	  break;
	  
	  /*
	   * Type = 0x000f: Session Length. (AIM)
	   * Type = 0x0010: Session Length. (AOL)
	   *
	   * The duration, in seconds, of the user's
	   * current session.
	   *
	   * Which TLV type this comes in depends
	   * on the service the user is using (AIM or AOL).
	   *
	   */
	case 0x000f:
	case 0x0010:
	  outinfo->sessionlen = aimutil_get32(&buf[i+4]);
	  break;

	  /*
	   * Reaching here indicates that either AOL has
	   * added yet another TLV for us to deal with, 
	   * or the parsing has gone Terribly Wrong.
	   *
	   * Either way, inform the owner and attempt
	   * recovery.
	   *
	   */
	default:
	  {
	    int len,z = 0, y = 0, x = 0;
	    char tmpstr[80];
	    printf("faim: userinfo: **warning: unexpected TLV:\n");
	    printf("faim: userinfo:   sn    =%s\n", outinfo->sn);
	    printf("faim: userinfo:   curtlv=0x%04x\n", curtlv);
	    printf("faim: userinfo:   type  =0x%04x\n",aimutil_get16(&buf[i]));
	    printf("faim: userinfo:   length=0x%04x\n", len = aimutil_get16(&buf[i+2]));
	    printf("faim: userinfo:   data: \n");
	    while (z<len)
	      {
		x = sprintf(tmpstr, "faim: userinfo:      ");
		for (y = 0; y < 8; y++)
		  {
		    if (z<len)
		      {
			sprintf(tmpstr+x, "%02x ", buf[i+4+z]);
			z++;
			x += 3;
		      }
		    else
		      break;
		  }
		printf("%s\n", tmpstr);
	      }
	  }
	  break;
	}  
      /*
       * No matter what, TLV triplets should always look like this:
       *
       *   u_short type;
       *   u_short length;
       *   u_char  data[length];
       *
       */
      i += (2 + 2 + aimutil_get16(&buf[i+2]));
      
      curtlv++;
    }
  
  return i;
}

/*
 * Oncoming Buddy notifications contain a subset of the
 * user information structure.  Its close enough to run
 * through aim_extractuserinfo() however.
 *
 */
int aim_parse_oncoming_middle(struct aim_session_t *sess,
			      struct command_rx_struct *command)
{
  struct aim_userinfo_s userinfo;
  u_int i = 0;
  rxcallback_t userfunc=NULL;

  i = 10;
  i += aim_extractuserinfo(command->data+i, &userinfo);

  userfunc = aim_callhandler(command->conn, AIM_CB_FAM_BUD, AIM_CB_BUD_ONCOMING);
  if (userfunc)
    i = userfunc(sess, command, &userinfo);

  return 1;
}

/*
 * Offgoing Buddy notifications contain no useful
 * information other than the name it applies to.
 *
 */
int aim_parse_offgoing_middle(struct aim_session_t *sess,
			      struct command_rx_struct *command)
{
  char sn[MAXSNLEN+1];
  u_int i = 0;
  rxcallback_t userfunc=NULL;

  /* Protect against future SN length extensions */
  strncpy(sn, command->data+11, (((int)(command->data+10))<=MAXSNLEN)?(int)command->data+10:MAXSNLEN);

  userfunc = aim_callhandler(command->conn, AIM_CB_FAM_BUD, AIM_CB_BUD_OFFGOING);
  if (userfunc)
    i = userfunc(sess, command, sn);

  return 1;
}

/*
 * This parses the user info stuff out all nice and pretty then calls 
 * the higher-level callback (in the user app).
 *
 */
int aim_parse_userinfo_middle(struct aim_session_t *sess,
			      struct command_rx_struct *command)
{
  struct aim_userinfo_s userinfo;
  char *prof_encoding = NULL;
  char *prof = NULL;
  u_int i = 0;
  rxcallback_t userfunc=NULL;
  struct aim_tlvlist_t *tlvlist;

  {
    u_long snacid = 0x000000000;
    struct aim_snac_t *snac = NULL;

    snacid = aimutil_get32(&command->data[6]);
    snac = aim_remsnac(sess, snacid);

    if (snac)
      {
	if (snac->data)
	  free(snac->data);
	else
	  printf("faim: parse_userinfo_middle: warning: no ->data in cached SNAC\n");
	free(snac);
      }
    else
      printf("faim: parseuserinfo_middle: warning: no SNAC cached with for this response (%08lx)\n", snacid);

  }
  
  i = 10;

  /*
   * extractuserinfo will give us the basic metaTLV information
   */
  i += aim_extractuserinfo(command->data+i, &userinfo);
  
  /*
   * However, in this command, there's usually more TLVs following...
   */ 
  tlvlist = aim_readtlvchain(command->data+i, command->commandlen-i);
  prof_encoding = aim_gettlv_str(tlvlist, 0x0001, 1);
  prof = aim_gettlv_str(tlvlist, 0x0002, 1);

  userfunc = aim_callhandler(command->conn, AIM_CB_FAM_LOC, AIM_CB_LOC_USERINFO);
  if (userfunc)
    {
      i = userfunc(sess,
		   command, 
		   &userinfo, 
		   prof_encoding, 
		   prof); 
    }
  
  free(prof_encoding);
  free(prof);
  aim_freetlvchain(&tlvlist);

  return 1;
}
