/* 
 *  -----------------------------------------------------------
 *  ProtoFAIM: v1.xx.xxplxx
 *  -----------------------------------------------------------
 *
 *  This is ProtoFAIM v1.xx.xxplxx!!! Its nearly completely 
 *  different than that ugly thing called v0.  This app is
 *  compatible with the latest version of the libfaim library.
 *  Work is continuing. 
 *
 *  ProtoFAIM should only be used for two things...
 *   1) Testing the libfaim backend.
 *   2) For reference on the libfaim API when developing clients.
 * 
 *  Its very ugly.  Probably always will be.  Nothing is more
 *  ugly than the backend itself, however.
 *
 *  -----------------------------------------------------------
 *
 *  I'm releasing this code and all it's associated linkage
 *  under the GNU General Public License.  For more information,
 *  please refer to http://www.fsf.org.  For any questions,
 *  please contact me at the address below.
 *
 *  Most everything:
 *  (c) 1998 Adam Fritzler, PST, afritz@iname.com
 *
 *  The password algorithms
 *  (c) 1998 Brock Wilcox, awwaiid@iname.com
 *
 *  THERE IS NO CODE FROM AOL'S AIM IN THIS CODE, NOR
 *  WAS THERE ANY DISASSEMBLAGE TO DEFINE PROTOCOL.  All
 *  information was gained through painstakingly comparing
 *  TCP dumps while the AIM Java client was running.  Nothing
 *  more than that, except for a lot of experimenting.
 *
 *  -----------------------------------------------------------
 *
 */

/*
  Current status:


 */

#define FAIMTEST_SCREENNAME "SN"
#define FAIMTEST_PASSWORD "PASS"

#include "aim.h" /* for struct defs, global ptrs, etc */

int faimtest_parse_oncoming(struct command_rx_struct *, ...);
int faimtest_parse_offgoing(struct command_rx_struct *, ...);
int faimtest_parse_login_phase3d_f(struct command_rx_struct *, ...);
int faimtest_auth_error(struct command_rx_struct *, ...);
int faimtest_auth_success(struct command_rx_struct *, ...);
int faimtest_parse_incoming_im(struct command_rx_struct *command, ...);
int faimtest_parse_userinfo(struct command_rx_struct *command, ...);
int faimtest_handleredirect(struct command_rx_struct *command, ...);
int faimtest_authsvrready(struct command_rx_struct *command, ...);
int faimtest_pwdchngdone(struct command_rx_struct *command, ...);
int faimtest_serverready(struct command_rx_struct *command, ...);
int faimtest_parse_misses(struct command_rx_struct *command, ...);
int main(void)
{

#if 0
  /*
    specify your custom command handlers here.  I recommend
    at least overriding the login_phase3d_f handler so that
    you can have your own buddy list and profile.  The
    rest are probably only useful to override for UI
    reasons.
   */
  rxcallback_t faimtest_callbacks[] = {
    faimtest_parse_incoming_im, /* incoming IM 0 */
    faimtest_parse_oncoming, /* oncoming buddy 1 */
    faimtest_parse_offgoing, /* offgoing buddy 2 */
    faimtest_parse_misses, /* AIM_CB_MISSED_IM 3 */
    faimtest_parse_misses, /* AIM_CB_MISSED_CALL 4 */
    NULL, /* login phase 4 packet C command 1 -- depricated 5 */
    NULL, /* login phase 4 packet C command 2 -- depricated 6 */
    NULL, /* login phase 2, first resp -- depricated 7 */
    faimtest_serverready, /* server ready -- **HANDLING REQUIRED** 8 */
    NULL, /* login phase 3 packet B -- depricated 9 */
    NULL, /* login phase 3D packet A -- depricated 10 */
    NULL, /* login phase 3D packet B -- depricated 11 */
    NULL, /* login phase 3D packet C -- depricated 12 */
    NULL, /* login phase 3D packet D -- depricated 13 */
    NULL, /* login phase 3D packet E -- depricated 14 */
    faimtest_handleredirect, /* redirect -- **HANDLING REQUIRED** 15 */
    faimtest_parse_misses, /* AIM_CB_RATECHANGE 16 */
    faimtest_parse_misses, /* AIM_CB_USERERROR 17 */
    NULL, /* completely unknown command 18 */
    faimtest_parse_userinfo, /* user info response 19 */
    NULL, /* user search by address response 20 */
    NULL, /* user serach by name response 21 */
    NULL, /* user search fail 22 */
    faimtest_auth_error, /* 23 */
    faimtest_auth_success, /* 24 */
    faimtest_authsvrready, /* 25 */
    NULL, /* 26 */
    faimtest_pwdchngdone, /* 27 */
    faimtest_serverready, /* 28 */
    0x00 /* terminating NULL -- REQUIRED 29 */
  };
#endif

  struct client_info_s info = {"FAIMtest (Hi guys!)", 3, 90, 42, "us", "en"};
  struct aim_conn_t *authconn = NULL;
  int stayconnected = 1;
  
  aim_connrst(); /* reset connection array -- there's a better place for this*/
  /* register our callback array (optional) */
  //aim_register_callbacks(faimtest_callbacks);

 enter:
  authconn = aim_newconn(AIM_CONN_TYPE_AUTH, FAIM_LOGIN_SERVER);
		       
  if (authconn == NULL)
    {
      fprintf(stderr, "faimtest: internal connection error while in aim_login.  bailing out.\n");
      return -1;
    }
  else if (authconn->fd == -1)
    {
      if (authconn->status & AIM_CONN_STATUS_RESOLVERR)
	fprintf(stderr, "faimtest: could not resolve authorizer name\n");
      else if (authconn->status & AIM_CONN_STATUS_CONNERR)
	fprintf(stderr, "faimtest: could not connect to authorizer\n");
      return -1;
    }
  else
    {
      aim_conn_addhandler(authconn, AIM_CB_FAM_GEN, AIM_CB_GEN_ERROR, faimtest_auth_error, 0);
      aim_conn_addhandler(authconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_AUTHSUCCESS, faimtest_auth_success, 0);
      aim_conn_addhandler(authconn, AIM_CB_FAM_GEN, AIM_CB_GEN_SERVERREADY, faimtest_authsvrready, 0);
      aim_send_login(authconn, FAIMTEST_SCREENNAME, FAIMTEST_PASSWORD, &info);
    }

  while (aim_select(NULL) > (struct aim_conn_t *)0)
    {
      if (aim_queue_outgoing)
	aim_tx_flushqueue();

      if (aim_get_command() < 0)
	{
	  printf("\afaimtest: connection error!\n");
	}
      else
	aim_rxdispatch();
    }

  /* Close up */
  printf("AIM just decided we didn't need to be here anymore, closing up.,,\n");
  
  /* close up all connections, dead or no */
  aim_logoff(); 

  if (stayconnected)
    {
      printf("\nTrying to reconnect in 2 seconds...\n");
      sleep(2);
      goto enter;
    }

  /* Get out */
  exit(0);
}

int faimtest_serverready(struct command_rx_struct *command, ...)
{
  switch (command->conn->type)
    {
    case AIM_CONN_TYPE_BOS:
      aim_bos_reqrate(command->conn); /* request rate info */
      aim_bos_ackrateresp(command->conn);  /* ack rate info response -- can we say timing? */
      aim_bos_setprivacyflags(command->conn, 0x00000003);
      
#if 0
      aim_bos_reqpersonalinfo(command->conn);
#endif
      
      aim_bos_reqservice(command->conn, AIM_CONN_TYPE_ADS); /* 0x05 == Advertisments */

#if 0
      aim_bos_reqrights(NULL);
      aim_bos_reqbuddyrights(NULL);
      aim_bos_reqlocaterights(NULL);
      aim_bos_reqicbmparaminfo(NULL);
#endif
      
      /* set group permissions */
      aim_bos_setgroupperm(NULL, 0x1f);
      fprintf(stderr, "faimtest: done with BOS ServerReady\n");
      break;
    case AIM_CONN_TYPE_CHATNAV:
      fprintf(stderr, "faimtest: chatnav: got server ready\n");
      break;
    default:
      fprintf(stderr, "faimtest: unknown connection type on Server Ready\n");
    }
  return 1;
}

/*
  handleredirect()...

  This, of course, handles Service Redirects from OSCAR.

  Should get passed in the following:
     struct command_rx_struct *command
       the raw command data
     int serviceid
       the destination service ID
     char *serverip
       the IP address of the service's server
     char *cookie
       the raw auth cookie
 */
int faimtest_handleredirect(struct command_rx_struct *command, ...)
{
  va_list ap;
  int serviceid;
  char *ip;
  char *cookie;

  /* this is the new buddy list */
  char buddies[] = "Buddy1&Buddy2&";
  /* this is the new profile */
  char profile[] = "Hello";  

  va_start(ap, command);
  serviceid = va_arg(ap, int);
  ip = va_arg(ap, char *);
  cookie = va_arg(ap, char *);
  va_end(ap);

  switch(serviceid)
    {
    case 0x0005: /* Advertisements */
      /*
       * The craziest explanation yet as to why we finish logging in when
       * we get the advertisements redirect, of which we don't use anyway....
       *                    IT WAS EASY!
       */

      /* send the buddy list and profile (required, even if empty) */
      aim_bos_setbuddylist(command->conn, buddies);
      aim_bos_setprofile(command->conn, profile);

      /* send final login command (required) */
      aim_bos_clientready(command->conn); /* tell BOS we're ready to go live */

      /* you should now be ready to go */
      printf("\nYou are now officially online. (%s)\n", ip);      

      break;
    case 0x0007: /* Authorizer */
      {
	struct aim_conn_t *tstconn;
	/* Open a connection to the Auth */
	tstconn = aim_newconn(AIM_CONN_TYPE_AUTH, ip);
	if ( (tstconn==NULL) || (tstconn->status >= AIM_CONN_STATUS_RESOLVERR) )
	  fprintf(stderr, "faimtest: unable to reconnect with authorizer\n");
	else
	  /* Send the cookie to the Auth */
	  aim_auth_sendcookie(tstconn, cookie);

      }  
      break;
    case 0x000d: /* ChatNav */
      {
	struct aim_conn_t *tstconn = NULL;
	tstconn = aim_newconn(AIM_CONN_TYPE_CHATNAV, ip);
	if ( (tstconn==NULL) || (tstconn->status >= AIM_CONN_STATUS_RESOLVERR))
	  {
	    fprintf(stderr, "faimtest: unable to connect to chatnav server\n");
	    return 1;
	  }
	aim_conn_addhandler(tstconn, AIM_CB_FAM_CTN, AIM_CB_SPECIAL_DEFAULT, aim_parse_unknown, 0);
	aim_conn_addhandler(tstconn, AIM_CB_FAM_GEN, AIM_CB_SPECIAL_DEFAULT, aim_parse_unknown, 0);
	aim_auth_sendcookie(tstconn, cookie);
	fprintf(stderr, "\achatnav: connected\n");
      }
      break;
    case 0x000e: /* Chat */
      {
#if 0
	struct aim_conn_t *tstconn = NULL;
	tstconn = aim_newconn(AIM_CONN_TYPE_CHAT, ip);
	if ( (tstconn==NULL) || (tstconn->status >= AIM_CONN_STATUS_RESOLVERR))
	  {
	    fprintf(stderr, "faimtest: unable to connect to chat server\n");
	    return 1;
	  }
	aim_auth_sendcookie(aim_getconn_type(AIM_CONN_TYPE_CHAT), cookie);
	fprintf(stderr, "\achat: connected\n");
#endif
      }
      break;
    default:
      printf("uh oh... got redirect for unknown service 0x%04x!!\n", serviceid);
      /* dunno */
    }

  return 1;
}

int faimtest_auth_error(struct command_rx_struct *command, ...)
{
  va_list ap;
  struct login_phase1_struct *logininfo;
  char *errorurl;
  short errorcode;

  va_start(ap, command);
  logininfo = va_arg(ap, struct login_phase1_struct *);
  printf("Screen name: %s\n", logininfo->screen_name);
  errorurl = va_arg(ap, char *);
  printf("Error URL: %s\n", errorurl);
  errorcode = va_arg(ap, short);
  printf("Error code: 0x%02x\n", errorcode);
  va_end(ap);

  aim_conn_close(aim_getconn_type(AIM_CONN_TYPE_AUTH));
  exit(0);
  
  return 0;
}

int faimtest_auth_success(struct command_rx_struct *command, ...)
{
  va_list ap;
  struct login_phase1_struct *logininfo;
  struct aim_conn_t *bosconn = NULL;

  va_start(ap, command);
  logininfo = va_arg(ap, struct login_phase1_struct *);
  va_end(ap);
  
  printf("Screen name: %s\n", logininfo->screen_name);
  printf("Reg status: %2d\n", logininfo->regstatus);
  printf("Email: %s\n", logininfo->email);
  printf("Cookie len: %d\n", sizeof(logininfo->cookie));
  printf("BOS IP: %s\n", logininfo->BOSIP);

  printf("Closing auth connection...\n");
  aim_conn_close(command->conn);
  bosconn = aim_newconn(AIM_CONN_TYPE_BOS, logininfo->BOSIP);
  if (bosconn == NULL)
    {
      fprintf(stderr, "faimtest: could not connect to BOS: internal error\n");
    }
  else if (bosconn->status != 0)
    {
      fprintf(stderr, "faimtest: could not connect to BOS\n");
    }
  else
    {
      aim_conn_addhandler(bosconn, AIM_CB_FAM_ACK, AIM_CB_ACK_ACK, NULL, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_SERVERREADY, faimtest_serverready, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_RATEINFO, NULL, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_REDIRECT, faimtest_handleredirect, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_MOTD, NULL, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_STS, AIM_CB_STS_SETREPORTINTERVAL, NULL, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_ONCOMING, faimtest_parse_oncoming, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_OFFGOING, faimtest_parse_offgoing, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_INCOMING, faimtest_parse_incoming_im, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_ERROR, faimtest_parse_misses, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_MISSEDCALL, faimtest_parse_misses, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_RATECHANGE, faimtest_parse_misses, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_ERROR, faimtest_parse_misses, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_USERINFO, faimtest_parse_userinfo, 0);

      aim_conn_addhandler(bosconn, AIM_CB_FAM_CTN, AIM_CB_CTN_DEFAULT, aim_parse_unknown, 0);
      aim_conn_addhandler(bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_DEFAULT, aim_parse_unknown, 0);
      aim_auth_sendcookie(bosconn, logininfo->cookie);
    }
  return 1;
}

int faimtest_parse_userinfo(struct command_rx_struct *command, ...)
{
  struct aim_userinfo_s *userinfo;
  char *prof_encoding = NULL;
  char *prof = NULL;

  va_list ap;
  va_start(ap, command);
  userinfo = va_arg(ap, struct aim_userinfo_s *);
  prof_encoding = va_arg(ap, char *);
  prof = va_arg(ap, char *);
  va_end(ap);
  
  printf("faimtest: userinfo: sn: %s\n", userinfo->sn);
  printf("faimtest: userinfo: warnlevel: 0x%04x\n", userinfo->warnlevel);
  printf("faimtest: userinfo: class: 0x%04x = ", userinfo->class);

  /*
   *  00000000  (binary)
   *         1  Trial  
   *        2   Unknown
   *       3    AOL
   *      4     Unknown
   *     5      Free
   * 
   * ORed together.
   *
   */

  if (userinfo->class & 0x0001)
    printf("TRIAL ");
  if (userinfo->class & 0x0002)
    printf("UNKNOWN_BIT2 ");
  if (userinfo->class & 0x0004)
    printf("AOL ");
  if (userinfo->class & 0x0008)
    printf("UNKNOWN_BIT4 ");
  if (userinfo->class & 0x0010)
    printf("FREE ");
  printf("\n");
  
  printf("faimtest: userinfo: membersince: %lu\n", userinfo->membersince);
  printf("faimtest: userinfo: onlinesince: %lu\n", userinfo->onlinesince);
  printf("faimtest: userinfo: idletime: 0x%04x\n", userinfo->idletime);
  
  printf("faimtest: userinfo: profile_encoding: %s\n", prof_encoding ? prof_encoding : "[none]");
  printf("faimtest: userinfo: prof: %s\n", prof ? prof : "[none]");
  
  return 1;
}

/*
 * The user-level Incoming ICBM callback.
 *
 * Arguments:
 *  struct command_rx_struct *  command     if you feel like doing it yourself
 *  char *                      srcsn       the source name
 *  char *                      msg         message
 *  int                         warnlevel   warning/evil level
 *  int                         class       user class
 *  ulong                       membersince time_t of date of signup
 *  ulong                       onsince     time_t of date of singon
 *  int                         idletime    min (sec?) idle
 *  int                         isautoreply TRUE if its an auto-response
 *
 */
int faimtest_parse_incoming_im(struct command_rx_struct *command, ...)
{
  struct aim_userinfo_s *userinfo;
  char *msg = NULL;
  int isautoreply = 0;
  va_list ap;
  char *tmpstr = NULL;

  va_start(ap, command);
  userinfo = va_arg(ap, struct aim_userinfo_s *);
  msg = va_arg(ap, char *);
  isautoreply = va_arg(ap, int);
  va_end(ap);

  printf("faimtest: icbm: sn = \"%s\"\n", userinfo->sn);
  printf("faimtest: icbm: warnlevel = 0x%04x\n", userinfo->warnlevel);
  printf("faimtest: icbm: class = 0x%04x ", userinfo->class);
  if (userinfo->class & 0x0010)
    printf("(FREE)\n");
  else if (userinfo->class & 0x0001)
    printf("(TRIAL)\n");
  else if (userinfo->class & 0x0004)
    printf("(AOL)\n");
  else
    printf("(UNKNOWN)\n");
  printf("faimtest: icbm: membersince = %lu\n", userinfo->membersince);
  printf("faimtest: icbm: onlinesince = %lu\n", userinfo->onlinesince);
  printf("faimtest: icbm: idletime = 0x%04x\n", userinfo->idletime);
  printf("faimtest: icbm: isautoreply = %s\n", isautoreply ? "TRUE" : "FALSE");  

  printf("faimtest: icbm: message: %s\n", msg);

  if (msg)
    {
      tmpstr = index(msg, '>');
      if (tmpstr != NULL)
	tmpstr+=1;
      else
	tmpstr = msg;
      
      if ( (strlen(tmpstr) >= 10) &&
	   (!strncmp(tmpstr, "disconnect", 10)) )
	{
	  aim_send_im(command->conn, "midendian", 0, "ta ta...");
	  aim_logoff();
	}
      else if (strstr(tmpstr, "goodday"))
	{
	  printf("faimtest: icbm: sending response\n");
	  aim_send_im(command->conn, userinfo->sn, 0, "Good day to you too.");
	}
#if 0
      else if (!strncmp(tmpstr, "joinchat", 8))
	{
	  aim_chat_join(command->conn, "GoodDay");
	}
#endif
      else 
	{
#if 0
	  printf("faimtest: icbm:  starting chat...\n");
	  aim_bos_reqservice(command->conn, AIM_CONN_TYPE_CHATNAV);
#else
	  aim_bos_setidle(command->conn, 0x0ffffffe);
#endif
	}

    }

  printf("faimtest: icbm: done with ICBM handling\n");

  return 1;
}

int faimtest_authsvrready(struct command_rx_struct *command, ...)
{
  printf("faimtest_authsvrready: called (contype: %d)\n", command->conn->type);
  sleep(10);
  /* should just be able to tell it we're ready too... */
  aim_auth_clientready(command->conn);

#if 0
  /*
   * This is where you'd really begin changing your password.
   *   However, this callback may get called for reasons other
   *   than you wanting to change your password.  You should 
   *   probably check that before actually doing it.
   */
  aim_auth_changepasswd(command->conn, "PWD1", "PWD2");
#endif

  return 1;
}

int faimtest_pwdchngdone(struct command_rx_struct *command, ...)
{
  printf("PASSWORD CHANGE SUCCESSFUL!!!\n");
  return 1;
}

int faimtest_parse_oncoming(struct command_rx_struct *command, ...)
{
  struct aim_userinfo_s *userinfo;
   
  va_list ap;
  va_start(ap, command);
  userinfo = va_arg(ap, struct aim_userinfo_s *);
  va_end(ap);

  printf("\n%s is now online\n", userinfo->sn);

  return 1;
}

int faimtest_parse_offgoing(struct command_rx_struct *command, ...)
{

  printf("\n%s has left\n", &(command->data[11]));

  return 1;
}


/* 
 * Handles callbacks for: AIM_CB_RATECHANGE, AIM_CB_USERERROR, 
 *   AIM_CB_MISSED_IM, and AIM_CB_MISSED_CALL.
 */
int faimtest_parse_misses(struct command_rx_struct *command, ...)
{
  u_short family;
  u_short subtype;

  family = (command->data[0] << 8) + command->data[1];
  subtype = (command->data[2] << 8) + command->data[3];
  
  switch (family)
    {
    case 0x0001:
      if (subtype == 0x000a) /* or AIM_CB_RATECHANGE */
	printf("\n****STOP SENDING/RECIEVING MESSAGES SO FAST!****\n\n");
      break;
    case 0x0002:
      if (subtype == 0x0001) /* or AIM_CB_USERERROR */
	{
	  u_long snacid = 0x00000000;
	  
	  snacid = aimutil_get32(&command->data[6]);
	  
	  printf("Received unknown error in SNAC family 0x0002 (snacid = %08lx)\n", snacid);
	}
      break;
    case 0x0004:
      if (subtype == 0x0001) /* or AIM_CB_MISSED_IM */
	printf("\n***LAST IM DIDN\'T MAKE IT BECAUSE THE BUDDY IS NOT ONLINE***\n\n");
      else if (subtype == 0x000a) /* or AIM_CB_MISSED_CALL */
	printf("You missed some messages from %s because they were sent too fast\n", &(command->data[13]));
      break;
    }

  return 0;
}





