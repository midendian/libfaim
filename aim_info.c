/*
 * aim_info.c
 *
 * The functions here are responsible for requesting and parsing information-
 * gathering SNACs.  
 *
 */


#include "aim.h" /* for most everything */

u_long aim_getinfo(struct aim_conn_t *conn, const char *sn)
{
  struct command_tx_struct newpacket;

  if (conn)
    newpacket.conn = conn;
  else
    newpacket.conn = aim_getconn_type(AIM_CONN_TYPE_BOS);

  newpacket.lock = 1;
  newpacket.type = 0x0002;

  newpacket.commandlen = 12 + 1 + strlen(sn);
  newpacket.data = (char *) malloc(newpacket.commandlen);

  aim_putsnac(newpacket.data, 0x0002, 0x0005, 0x0000, aim_snac_nextid);

  aimutil_put16(newpacket.data+10, 0x0001);
  aimutil_put8(newpacket.data+12, strlen(sn));
  aimutil_putstr(newpacket.data+13, sn, strlen(sn));

  aim_tx_enqueue(&newpacket);

  {
    struct aim_snac_t snac;
    
    snac.id = aim_snac_nextid;
    snac.family = 0x0002;
    snac.type = 0x0005;
    snac.flags = 0x0000;

    snac.data = malloc(strlen(sn)+1);
    memcpy(snac.data, sn, strlen(sn)+1);

    aim_newsnac(&snac);
  }

  return (aim_snac_nextid++);
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
int aim_parse_oncoming_middle(struct command_rx_struct *command)
{
  struct aim_userinfo_s userinfo;
  u_int i = 0;
  rxcallback_t userfunc=NULL;

  i = 10;
  i += aim_extractuserinfo(command->data+i, &userinfo);

  userfunc = aim_callhandler(command->conn, AIM_CB_FAM_BUD, AIM_CB_BUD_ONCOMING);
  if (userfunc)
    i = userfunc(command, &userinfo);

  return 1;
}


/*
 * This parses the user info stuff out all nice and pretty then calls 
 * the higher-level callback (in the user app).
 *
 */
int aim_parse_userinfo_middle(struct command_rx_struct *command)
{
  struct aim_userinfo_s userinfo;
  char *prof_encoding = NULL;
  char *prof = NULL;
  u_int i = 0;
  rxcallback_t userfunc=NULL;

  {
    u_long snacid = 0x000000000;
    struct aim_snac_t *snac = NULL;

    snacid = aimutil_get32(&command->data[6]);
    snac = aim_remsnac(snacid);

    free(snac->data);
    free(snac);

  }
  
  i = 10;
  i += aim_extractuserinfo(command->data+i, &userinfo);

  if (i < command->commandlen)
    {
      if (aimutil_get16(&command->data[i]) == 0x0001)
        {
          int len = 0;

	  len = aimutil_get16(&command->data[i+2]);

          prof_encoding = (char *) malloc(len+1);
          memcpy(prof_encoding, &(command->data[i+4]), len);
          prof_encoding[len] = '\0';

          i += (2+2+len);
        }
      else
        {
          printf("faim: userinfo: **warning: unexpected TLV after TLVblock t(%02x%02x) l(%02x%02x)\n", command->data[i], command->data[i+1], command->data[i+2], command->data[i+3]);
          i += 2 + 2 + command->data[i+3];
        }
    }

  if (i < command->commandlen)
    {
      if (aimutil_get16(&command->data[i]) == 0x0002)
	{
	  int len = 0;
	  len = aimutil_get16(&command->data[i+2]);
	  
	  prof = (char *) malloc(len+1);
	  memcpy(prof, &(command->data[i+4]), len);
	  prof[len] = '\0';
	}
      else
	printf("faim:userinfo: **warning: profile not found, but still have data\n");
    }

  userfunc = aim_callhandler(command->conn, AIM_CB_FAM_LOC, AIM_CB_LOC_USERINFO);
  if (userfunc)
    {
      i = userfunc(command, 
		   &userinfo, 
		   prof_encoding, 
		   prof); 
    }

  free(prof_encoding);
  free(prof);

  return 1;
}
