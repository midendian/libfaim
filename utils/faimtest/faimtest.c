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

#include <faim/aim.h> 

int faimtest_parse_oncoming(struct aim_session_t *, struct command_rx_struct *, ...);
int faimtest_parse_offgoing(struct aim_session_t *, struct command_rx_struct *, ...);
int faimtest_parse_login_phase3d_f(struct aim_session_t *, struct command_rx_struct *, ...);
int faimtest_parse_authresp(struct aim_session_t *, struct command_rx_struct *, ...);
int faimtest_parse_incoming_im(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_parse_userinfo(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_handleredirect(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_authsvrready(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_pwdchngdone(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_serverready(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_parse_misses(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_parse_motd(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_parse_login(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_chatnav_info(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_chat_incomingmsg(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_chat_infoupdate(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_chat_leave(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_chat_join(struct aim_session_t *sess, struct command_rx_struct *command, ...);

static char *screenname,*password;

int main(void)
{
  struct aim_session_t aimsess;
  struct aim_conn_t *authconn = NULL;
  int stayconnected = 1;
  struct client_info_s info = {"FAIMtest (Hi guys!)", 3, 5, 1670, "us", "en"};
    
  aim_session_init(&aimsess);

  if ( !(screenname = getenv("SCREENNAME")) ||
       !(password = getenv("PASSWORD")))
    {
      printf("Must specify SCREENAME and PASSWORD in environment.\n");
      return -1;
    }

  /*
   * (I used a goto-based loop here because n wanted quick proof
   *  that reconnecting without restarting was actually possible...)
   */
 enter:
  authconn = aim_newconn(&aimsess, AIM_CONN_TYPE_AUTH, FAIM_LOGIN_SERVER);

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
#ifdef SNACLOGIN
      /* new login code -- not default -- pending new password encryption algo */
      aim_conn_addhandler(&aimsess, authconn, 0x0017, 0x0007, faimtest_parse_login, 0);
      aim_conn_addhandler(&aimsess, authconn, 0x0017, 0x0003, faimtest_parse_authresp, 0);

      aim_sendconnack(&aimsess, authconn);
      aim_request_login(&aimsess, authconn, FAIMTEST_SCREENNAME);
#else
      aim_conn_addhandler(&aimsess, authconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_AUTHSUCCESS, faimtest_parse_authresp, 0);
      aim_conn_addhandler(&aimsess, authconn, AIM_CB_FAM_GEN, AIM_CB_GEN_SERVERREADY, faimtest_authsvrready, 0);
      aim_send_login(&aimsess, authconn, screenname, password, &info);
 
#endif
    }

  while (aim_select(&aimsess, NULL) > (struct aim_conn_t *)0)
    {
      if (aimsess.queue_outgoing)
	aim_tx_flushqueue(&aimsess);

      if (aim_get_command(&aimsess) < 0)
	{
	  printf("\afaimtest: connection error!\n");
	}
      else
	aim_rxdispatch(&aimsess);
    }

  /* Close up */
  printf("AIM just decided we didn't need to be here anymore, closing up.,,\n");
  
  /* close up all connections, dead or no */
  aim_logoff(&aimsess); 

  if (stayconnected)
    {
      printf("\nTrying to reconnect in 2 seconds...\n");
      sleep(2);
      goto enter;
    }

  /* Get out */
  exit(0);
}

int faimtest_serverready(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  switch (command->conn->type)
    {
    case AIM_CONN_TYPE_BOS:

      aim_bos_reqrate(sess, command->conn); /* request rate info */
      aim_bos_ackrateresp(sess, command->conn);  /* ack rate info response -- can we say timing? */
      aim_bos_setprivacyflags(sess, command->conn, 0x00000003);
      
#if 0
      aim_bos_reqpersonalinfo(sess, command->conn);
#endif
      
      /* Request advertisement service -- see comment in handleredirect */
      aim_bos_reqservice(sess, command->conn, AIM_CONN_TYPE_ADS);
      aim_setversions(sess, command->conn);

#if 0
      aim_bos_reqrights(sess, command->conn);
      aim_bos_reqbuddyrights(sess, command->conn);
      aim_bos_reqlocaterights(sess, command->conn);
      aim_bos_reqicbmparaminfo(sess, command->conn);
#endif
      
      /* set group permissions */
      aim_bos_setgroupperm(sess, command->conn, 0x1f);
      fprintf(stderr, "faimtest: done with BOS ServerReady\n");
      break;

    case AIM_CONN_TYPE_CHATNAV:
      fprintf(stderr, "faimtest: chatnav: got server ready\n");
      aim_conn_addhandler(sess, command->conn, AIM_CB_FAM_CTN, AIM_CB_CTN_INFO, faimtest_chatnav_info, 0);
      aim_bos_reqrate(sess, command->conn);
      aim_bos_ackrateresp(sess, command->conn);
      aim_chatnav_clientready(sess, command->conn);
      aim_chatnav_reqrights(sess, command->conn);

      break;
    case AIM_CONN_TYPE_CHAT:
      aim_conn_addhandler(sess, command->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_USERJOIN, faimtest_chat_join, 0);
      aim_conn_addhandler(sess, command->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_USERLEAVE, faimtest_chat_leave, 0);
      aim_conn_addhandler(sess, command->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_ROOMINFOUPDATE, faimtest_chat_infoupdate, 0);
      aim_conn_addhandler(sess, command->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_INCOMINGMSG, faimtest_chat_incomingmsg, 0);
      aim_bos_reqrate(sess, command->conn);
      aim_bos_ackrateresp(sess, command->conn);
      aim_chat_clientready(sess, command->conn);
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
int faimtest_handleredirect(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  int serviceid;
  char *ip;
  char *cookie;

  /* this is the new buddy list */
  char buddies[] = "Buddy1&Buddy2&ThisHereIsAName2&";
  /* this is the new profile */
  char profile[] = "Hello";  

  va_start(ap, command);
  serviceid = va_arg(ap, int);
  ip = va_arg(ap, char *);
  cookie = va_arg(ap, char *);
 
  switch(serviceid)
    {
    case 0x0005: /* Advertisements */
      /*
       * The craziest explanation yet as to why we finish logging in when
       * we get the advertisements redirect, of which we don't use anyway....
       *                    IT WAS EASY!
       */

      /* send the buddy list and profile (required, even if empty) */
      aim_bos_setbuddylist(sess, command->conn, buddies);
      aim_bos_setprofile(sess, command->conn, profile, NULL);

      /* send final login command (required) */
      aim_bos_clientready(sess, command->conn); /* tell BOS we're ready to go live */

      /* you should now be ready to go */
      printf("\nYou are now officially online.\n");      

      break;
    case 0x0007: /* Authorizer */
      {
	struct aim_conn_t *tstconn;
	/* Open a connection to the Auth */
	tstconn = aim_newconn(sess, AIM_CONN_TYPE_AUTH, ip);
	if ( (tstconn==NULL) || (tstconn->status >= AIM_CONN_STATUS_RESOLVERR) )
	  fprintf(stderr, "faimtest: unable to reconnect with authorizer\n");
	else
	  /* Send the cookie to the Auth */
	  aim_auth_sendcookie(sess, tstconn, cookie);

      }  
      break;
    case 0x000d: /* ChatNav */
      {
	struct aim_conn_t *tstconn = NULL;
	tstconn = aim_newconn(sess, AIM_CONN_TYPE_CHATNAV, ip);
	if ( (tstconn==NULL) || (tstconn->status >= AIM_CONN_STATUS_RESOLVERR))
	  {
	    fprintf(stderr, "faimtest: unable to connect to chatnav server\n");
	    return 1;
	  }
#if 0
	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_CTN, AIM_CB_SPECIAL_DEFAULT, aim_parse_unknown, 0);
	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_GEN, AIM_CB_SPECIAL_DEFAULT, aim_parse_unknown, 0);
#endif
	aim_conn_addhandler(sess, tstconn, 0x0001, 0x0003, faimtest_serverready, 0);
	aim_auth_sendcookie(sess, tstconn, cookie);
	fprintf(stderr, "\achatnav: connected\n");
      }
      break;
    case 0x000e: /* Chat */
      {
	char *roomname = NULL;
	struct aim_conn_t *tstconn = NULL;

	roomname = va_arg(ap, char *);

	tstconn = aim_newconn(sess, AIM_CONN_TYPE_CHAT, ip);
	if ( (tstconn==NULL) || (tstconn->status >= AIM_CONN_STATUS_RESOLVERR))
	  {
	    fprintf(stderr, "faimtest: unable to connect to chat server\n");
	    return 1;
	  }		
	printf("faimtest: chat: connected\n");

	/*
	 * We must do this to attach the stored name to the connection!
	 */
	aim_chat_attachname(tstconn, roomname);

	aim_conn_addhandler(sess, tstconn, 0x0001, 0x0003, faimtest_serverready, 0);
	aim_auth_sendcookie(sess, tstconn, cookie);
      }
      break;
    default:
      printf("uh oh... got redirect for unknown service 0x%04x!!\n", serviceid);
      /* dunno */
    }

  va_end(ap);

  return 1;
}

int faimtest_parse_authresp(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  struct aim_conn_t *bosconn = NULL;
  

  printf("Screen name: %s\n", sess->logininfo.screen_name);

  /*
   * Check for error.
   */
  if (sess->logininfo.errorcode)
    {
      printf("Login Error Code 0x%04x\n", sess->logininfo.errorcode);
      printf("Error URL: %s\n", sess->logininfo.errorurl);
      aim_conn_close(command->conn);
      exit(0); /* XXX: should return in order to let the above things get free()'d. */
    }

  printf("Reg status: %2d\n", sess->logininfo.regstatus);
  printf("Email: %s\n", sess->logininfo.email);
  printf("BOS IP: %s\n", sess->logininfo.BOSIP);

  printf("Closing auth connection...\n");
  aim_conn_close(command->conn);
  bosconn = aim_newconn(sess, AIM_CONN_TYPE_BOS, sess->logininfo.BOSIP);
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
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_ACK, AIM_CB_ACK_ACK, NULL, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_SERVERREADY, faimtest_serverready, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_RATEINFO, NULL, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_REDIRECT, faimtest_handleredirect, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_STS, AIM_CB_STS_SETREPORTINTERVAL, NULL, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_ONCOMING, faimtest_parse_oncoming, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_OFFGOING, faimtest_parse_offgoing, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_INCOMING, faimtest_parse_incoming_im, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_ERROR, faimtest_parse_misses, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_MISSEDCALL, faimtest_parse_misses, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_RATECHANGE, faimtest_parse_misses, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_ERROR, faimtest_parse_misses, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_USERINFO, faimtest_parse_userinfo, 0);

      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_CTN, AIM_CB_CTN_DEFAULT, aim_parse_unknown, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_DEFAULT, aim_parse_unknown, 0);
      aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_MOTD, faimtest_parse_motd, 0);
      aim_auth_sendcookie(sess, bosconn, sess->logininfo.cookie);
    }
  return 1;
}

int faimtest_parse_userinfo(struct aim_session_t *sess, struct command_rx_struct *command, ...)
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
 *  u_int                       icbmflags   sets AIM_IMFLAGS_{AWAY,ACK}
 *
 */
int faimtest_parse_incoming_im(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  int channel;
  va_list ap;

  va_start(ap, command);
  channel = va_arg(ap, int);

  /*
   * Channel 1: Standard Message
   */
  if (channel == 1)
    {
      struct aim_userinfo_s *userinfo;
      char *msg = NULL;
      u_int icbmflags = 0;
      char *tmpstr = NULL;
      u_short flag1, flag2;
  
      userinfo = va_arg(ap, struct aim_userinfo_s *);
      msg = va_arg(ap, char *);
      icbmflags = va_arg(ap, u_int);
      flag1 = va_arg(ap, u_short);
      flag2 = va_arg(ap, u_short);
      va_end(ap);
      
      printf("faimtest: icbm: sn = \"%s\"\n", userinfo->sn);
      printf("faimtest: icbm: warnlevel = 0x%04x\n", userinfo->warnlevel);
      printf("faimtest: icbm: class = 0x%04x ", userinfo->class);
      if (userinfo->class & 0x0010)
	printf("(FREE) ");
      if (userinfo->class & 0x0001)
	printf("(TRIAL) ");
      if (userinfo->class & 0x0004)
	printf("(AOL) ");
      printf("\n");
      printf("faimtest: icbm: membersince = %lu\n", userinfo->membersince);
      printf("faimtest: icbm: onlinesince = %lu\n", userinfo->onlinesince);
      printf("faimtest: icbm: idletime = 0x%04x\n", userinfo->idletime);
      
      printf("faimtest: icbm: icbmflags = ");
      if (icbmflags & AIM_IMFLAGS_AWAY)
	printf("away ");
      if (icbmflags & AIM_IMFLAGS_ACK)
	printf("ackrequest ");
      printf("\n");
      
      printf("faimtest: icbm: encoding flags = {%04x, %04x}\n", flag1, flag2);
      
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
	      aim_send_im(sess, command->conn, "midendian", 0, "ta ta...");
	      aim_logoff(sess);
	    }
	  else if (strstr(tmpstr, "goodday"))
	    {
	      printf("faimtest: icbm: sending response\n");
	      aim_send_im(sess, command->conn, userinfo->sn, 0, "Good day to you too.");
	    }
	  else if (!strncmp(tmpstr, "open chatnav", 12))
	    {
	      aim_bos_reqservice(sess, command->conn, AIM_CONN_TYPE_CHATNAV);
	      //aim_chat_join(sess, command->conn, "thishereisaname2_chat85");
	    }
	  else if (!strncmp(tmpstr, "create", 6))
	    {
	      aim_chatnav_createroom(sess, aim_getconn_type(sess, AIM_CONN_TYPE_CHATNAV), "WorldDomination", 0x0004);
	    }
	  else if (!strncmp(tmpstr, "close chatnav", 13))
	    aim_conn_close(aim_getconn_type(sess, AIM_CONN_TYPE_CHATNAV));
	  else if (!strncmp(tmpstr, "join", 4))
	    {
	      aim_chat_join(sess, command->conn, 0x0004, "worlddomination");
	    }
	  else if (!strncmp(tmpstr, "leave", 5))
	    aim_chat_leaveroom(sess, "worlddomination");
	  else 
	    {
#if 0
	      printf("faimtest: icbm:  starting chat...\n");
	      aim_bos_reqservice(sess, command->conn, AIM_CONN_TYPE_CHATNAV);
#else
	      aim_bos_setidle(sess, command->conn, 0x0ffffffe);
#endif
	    }
	  
	}
    }
  /*
   * Channel 2: Rendevous Request
   */
  else if (channel == 2)
    {
      struct aim_userinfo_s *userinfo;
      int rendtype = 0;

      rendtype = va_arg(ap, int);
      if (rendtype == 0)
	{
	  char *msg,*encoding,*lang;
	  struct aim_chat_roominfo *roominfo;
	  
	  userinfo = va_arg(ap, struct aim_userinfo_s *);
	  roominfo = va_arg(ap, struct aim_chat_roominfo *);
	  msg = va_arg(ap, char *);
	  encoding = va_arg(ap, char *);
	  lang = va_arg(ap, char *);
	  va_end(ap);
	  
	  printf("faimtest: chat invitation: source sn = %s\n", userinfo->sn);
	  printf("faimtest: chat invitation: \twarnlevel = 0x%04x\n", userinfo->warnlevel);
	  printf("faimtest: chat invitation: \tclass = 0x%04x ", userinfo->class);
	  if (userinfo->class & 0x0010)
	    printf("(FREE) ");
	  if (userinfo->class & 0x0001)
	    printf("(TRIAL) ");
	  if (userinfo->class & 0x0004)
	    printf("(AOL) ");
	  printf("\n");
	  /* we dont get membersince on chat invites! */
	  printf("faimtest: chat invitation: \tonlinesince = %lu\n", userinfo->onlinesince);
	  printf("faimtest: chat invitation: \tidletime = 0x%04x\n", userinfo->idletime);
	  
	  printf("faimtest: chat invitation: message = %s\n", msg);
	  printf("faimtest: chat invitation: room name = %s\n", roominfo->name);
	  printf("faimtest: chat invitation: encoding = %s\n", encoding);
	  printf("faimtest: chat invitation: language = %s\n", lang);
	  printf("faimtest: chat invitation: exchange = 0x%04x\n", roominfo->exchange);
	  printf("faimtest: chat invitation: instance = 0x%04x\n", roominfo->instance);
	  printf("faimtest: chat invitiation: autojoining %s...\n", roominfo->name);
	  /*
	   * Automatically join room...
	   */ 
	  aim_chat_join(sess, command->conn, 0x0004, roominfo->name);
	}	
      else if (rendtype == 1)
	{
	  userinfo = va_arg(ap, struct aim_userinfo_s *);
	  va_end(ap);
	  
	  printf("faimtest: voice invitation: source sn = %s\n", userinfo->sn);
	  printf("faimtest: voice invitation: \twarnlevel = 0x%04x\n", userinfo->warnlevel);
	  printf("faimtest: voice invitation: \tclass = 0x%04x ", userinfo->class);
	  if (userinfo->class & 0x0010)
	    printf("(FREE) ");
	  if (userinfo->class & 0x0001)
	    printf("(TRIAL) ");
	  if (userinfo->class & 0x0004)
	    printf("(AOL) ");
	  printf("\n");
	  /* we dont get membersince on chat invites! */
	  printf("faimtest: voice invitation: \tonlinesince = %lu\n", userinfo->onlinesince);
	  printf("faimtest: voice invitation: \tidletime = 0x%04x\n", userinfo->idletime);
	  
	}
      else
	printf("faimtest: icbm: unknown rendtype (%d)\n", rendtype);
    }
  else
    printf("faimtest does not support channels > 2 (chan = %02x)\n", channel);
  printf("faimtest: icbm: done with ICBM handling\n");

  return 1;
}

int faimtest_authsvrready(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  printf("faimtest_authsvrready: called (contype: %d)\n", command->conn->type);
  sleep(10);
  /* should just be able to tell it we're ready too... */
  aim_auth_clientready(sess, command->conn);

#if 0
  /*
   * This is where you'd really begin changing your password.
   *   However, this callback may get called for reasons other
   *   than you wanting to change your password.  You should 
   *   probably check that before actually doing it.
   */
  aim_auth_changepasswd(sess, command->conn, "PWD1", "PWD2");
#endif

  return 1;
}

int faimtest_pwdchngdone(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  printf("PASSWORD CHANGE SUCCESSFUL!!!\n");
  return 1;
}

int faimtest_parse_oncoming(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  struct aim_userinfo_s *userinfo;
   
  va_list ap;
  va_start(ap, command);
  userinfo = va_arg(ap, struct aim_userinfo_s *);
  va_end(ap);

  printf("\n%s is now online (class: %04x = %s%s%s%s%s%s%s%s)\n", userinfo->sn, userinfo->class,
	 (userinfo->class&AIM_CLASS_TRIAL)?" TRIAL":"",
	 (userinfo->class&AIM_CLASS_UNKNOWN2)?" UNKNOWN2":"",
	 (userinfo->class&AIM_CLASS_AOL)?" AOL":"",
	 (userinfo->class&AIM_CLASS_UNKNOWN4)?" UNKNOWN4":"",
	 (userinfo->class&AIM_CLASS_FREE)?" FREE":"",
	 (userinfo->class&AIM_CLASS_AWAY)?" AWAY":"",
	 (userinfo->class&AIM_CLASS_UNKNOWN40)?" UNKNOWN40":"",
	 (userinfo->class&AIM_CLASS_UNKNOWN80)?" UNKNOWN80":"");

  return 1;
}

int faimtest_parse_offgoing(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  char *sn;
  va_list ap;
  
  va_start(ap, command);
  sn = va_arg(ap, char *);
  va_end(ap);

  printf("\n%s has left\n", sn);

  return 1;
}

int faimtest_parse_motd(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  char *msg;
  u_short id;
  va_list ap;
  
  va_start(ap, command);
  id = va_arg(ap, u_short);
  msg = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: motd: %s\n", msg);

  return 1;
}

/* 
 * Handles callbacks for: AIM_CB_RATECHANGE, AIM_CB_USERERROR, 
 *   AIM_CB_MISSED_IM, and AIM_CB_MISSED_CALL.
 */
int faimtest_parse_misses(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  u_short family;
  u_short subtype;

  family = aimutil_get16(command->data+0);
  subtype= aimutil_get16(command->data+2);
  
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

#ifdef SNACLOGIN
int faimtest_parse_login(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  struct client_info_s info = {"FAIMtest (Hi guys!)", 3, 5, 1670, "us", "en"};
  u_char authcookie[11];
  int i;
  
  for (i = 0; i < (int)command->data[11]; i++)
    authcookie[i] = command->data[12+i];
  authcookie[i] = '\0';

  printf("faimtest: logincookie: %s\n", authcookie);
  
  aim_send_login(sess, command->conn, FAIMTEST_SCREENNAME, FAIMTEST_PASSWORD, &info);
 
  return 1;
}
#endif

int faimtest_chat_join(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  struct aim_userinfo_s *userinfo;
  int count = 0, i = 0;
  
  va_start(ap, command);
  count = va_arg(ap, int);
  userinfo = va_arg(ap, struct aim_userinfo_s *);
  va_end(ap);

  printf("faimtest: chat: %s:  New occupants have joined:\n", (char *)command->conn->priv);
  while (i < count)
    printf("faimtest: chat: %s: \t%s\n", (char *)command->conn->priv, userinfo[i++].sn);

  return 1;
}

int faimtest_chat_leave(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  struct aim_userinfo_s *userinfo;
  int count = 0, i = 0;
  
  va_start(ap, command);
  count = va_arg(ap, int);
  userinfo = va_arg(ap, struct aim_userinfo_s *);
  va_end(ap);

  printf("faimtest: chat: %s:  Some occupants have left:\n", (char *)command->conn->priv);
  while (i < count)
    printf("faimtest: chat: %s: \t%s\n", (char *)command->conn->priv, userinfo[i++].sn);

  return 1;
}

int faimtest_chat_infoupdate(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  struct aim_userinfo_s *userinfo;
  struct aim_chat_roominfo *roominfo;
  char *roomname;
  int usercount,i;
  char *roomdesc;

  va_start(ap, command);
  roominfo = va_arg(ap, struct aim_chat_roominfo *);
  roomname = va_arg(ap, char *);
  usercount= va_arg(ap, int);
  userinfo = va_arg(ap, struct aim_userinfo_s *);
  roomdesc = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: chat: %s:  info update:\n", (char *)command->conn->priv);
  printf("faimtest: chat: %s:  \tRoominfo: {%04x, %s, %04x}\n", 
	 (char *)command->conn->priv,
	 roominfo->exchange,
	 roominfo->name,
	 roominfo->instance);
  printf("faimtest: chat: %s:  \tRoomname: %s\n", (char *)command->conn->priv, roomname);
  printf("faimtest: chat: %s:  \tRoomdesc: %s\n", (char *)command->conn->priv, roomdesc);
  printf("faimtest: chat: %s:  \tOccupants: (%d)\n", (char *)command->conn->priv, usercount);
  
  i = 0;
  while (i < usercount)
    printf("faimtest: chat: %s:  \t\t%s\n", (char *)command->conn->priv, userinfo[i++].sn);

  return 1;
}

int faimtest_chat_incomingmsg(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  struct aim_userinfo_s *userinfo;
  char *msg;
  char tmpbuf[1152];
 
  va_start(ap, command);
  userinfo = va_arg(ap, struct aim_userinfo_s *);	
  msg = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: chat: %s: incoming msg from %s: %s\n", (char *)command->conn->priv, userinfo->sn, msg);

  /*
   * Do an echo for testing purposes.  But not for ourselves ("oops!")
   */
  if (strcmp(userinfo->sn, sess->logininfo.screen_name) != 0)
    {
      sprintf(tmpbuf, "(%s said \"%s\")", userinfo->sn, msg);
      aim_chat_send_im(sess, command->conn, tmpbuf);
    }

  return 1;
}

int faimtest_chatnav_info(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  u_short type;
  va_list ap;

  ap = va_start(ap, command);
  type = va_arg(ap, u_short);

  switch(type)
    {
    case 0x0002:
      {
	int maxrooms;
	struct aim_chat_exchangeinfo *exchanges;
	int exchangecount,i = 0;
	
	maxrooms = va_arg(ap, u_char);
	exchangecount = va_arg(ap, int);
	exchanges = va_arg(ap, struct aim_chat_exchangeinfo *);
	va_end(ap);

	printf("faimtest: chat info: Chat Rights:\n");
	printf("faimtest: chat info: \tMax Concurrent Rooms: %d\n", maxrooms);
	
	printf("faimtest: chat info: \tExchange List: (%d total)\n", exchangecount);
	while (i < exchangecount)
	  {
	    printf("faimtest: chat info: \t\t%x: %s (%s/%s)\n", 
		   exchanges[i].number,	
		   exchanges[i].name,
		   exchanges[i].charset1,
		   exchanges[i].lang1);
	    i++;
	  }
	
      }
      break;
    default:
      va_end(ap);
      printf("faimtest: chatnav info: unknown type (%04x)\n", type);
    }
  return 1;
}
