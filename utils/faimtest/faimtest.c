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
int faimtest_parse_msgack(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_parse_motd(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_parse_login(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_chatnav_info(struct aim_session_t *, struct command_rx_struct *command, ...);
int faimtest_chat_incomingmsg(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_chat_infoupdate(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_chat_leave(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_chat_join(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_parse_connerr(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_debugconn_connect(struct aim_session_t *sess, struct command_rx_struct *command, ...);

int faimtest_directim_request(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_directim_initiate(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_directim_connect(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_directim_incoming(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_directim_disconnect(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_directim_typing(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_parse_ratechange(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_parse_evilnotify(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_parse_msgerr(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_parse_buddyrights(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_parse_locerr(struct aim_session_t *sess, struct command_rx_struct *command, ...);

static char *msgerrreasons[] = {
  "Invalid error",
  "Invalid SNAC",
  "Rate to host",
  "Rate to client",
  "Not logged on",
  "Service unavailable",
  "Service not defined",
  "Obsolete SNAC",
  "Not supported by host",
  "Not supported by client",
  "Refused by client",
  "Reply too big",
  "Responses lost",
  "Request denied",
  "Busted SNAC payload",
  "Insufficient rights",
  "In local permit/deny",
  "Too evil (sender)",
  "Too evil (receiver)",
  "User temporarily unavailable",
  "No match",
  "List overflow",
  "Request ambiguous",
  "Queue full",
  "Not while on AOL"};
static int msgerrreasonslen = 25;

int faimtest_reportinterval(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  if (command->data) {
    printf("aim: minimum report interval: %d (seconds?)\n", aimutil_get16(command->data+10));
  } else
    printf("aim: NULL minimum report interval!\n");
  return 1;
}

#ifdef _WIN32
/*
 * This is really all thats needed to link against libfaim on win32.
 *
 * Note that this particular version of faimtest has never been tested
 * on win32, but I'm fairly sure it should.
 */
int initwsa(void)
{
  WORD wVersionRequested;
  WSADATA wsaData;

  wVersionRequested = MAKEWORD(2,2);
  return WSAStartup(wVersionRequested, &wsaData);
}
#endif /* _WIN32 */

static char *screenname,*password,*server=NULL;

int main(void)
{
  struct aim_session_t aimsess;
  struct aim_conn_t *authconn = NULL, *waitingconn = NULL;
  int keepgoing = 1;

  int selstat = 0;

  if ( !(screenname = getenv("SCREENNAME")) ||
       !(password = getenv("PASSWORD")))
    {
      printf("Must specify SCREENAME and PASSWORD in environment.\n");
      return -1;
    }

  server = getenv("AUTHSERVER");

#ifdef _WIN32
  if (initwsa() != 0) {
    printf("faimtest: could not initialize windows sockets\n");
    return -1;
  }
#endif /* _WIN32 */

  aim_session_init(&aimsess);

  authconn = aim_newconn(&aimsess, AIM_CONN_TYPE_AUTH, server?server:FAIM_LOGIN_SERVER);

  if (authconn == NULL) {
    fprintf(stderr, "faimtest: internal connection error while in aim_login.  bailing out.\n");
    return -1;
  } else if (authconn->fd == -1) {
    if (authconn->status & AIM_CONN_STATUS_RESOLVERR)
      fprintf(stderr, "faimtest: could not resolve authorizer name\n");
    else if (authconn->status & AIM_CONN_STATUS_CONNERR)
      fprintf(stderr, "faimtest: could not connect to authorizer\n");
    aim_conn_kill(&aimsess, &authconn);
    return -1;
  }

  aim_conn_addhandler(&aimsess, authconn, 0x0017, 0x0007, faimtest_parse_login, 0);
  aim_conn_addhandler(&aimsess, authconn, 0x0017, 0x0003, faimtest_parse_authresp, 0);
    
  aim_sendconnack(&aimsess, authconn);
  aim_request_login(&aimsess, authconn, screenname);

  aim_conn_addhandler(&aimsess, authconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_DEBUGCONN_CONNECT, faimtest_debugconn_connect, 0);

  printf("faimtest: login request sent\n");

  while (keepgoing) {
    waitingconn = aim_select(&aimsess, NULL, &selstat);

    switch(selstat) {
    case -1: /* error */
      keepgoing = 0; /* fall through and hit the aim_logoff() */
      break;

    case 0: /* no events pending */
      break;

    case 1: /* outgoing data pending */
      aim_tx_flushqueue(&aimsess);
      break;

    case 2: /* incoming data pending */
      if (waitingconn->type == AIM_CONN_TYPE_RENDEZVOUS_OUT) {
	if (aim_handlerendconnect(&aimsess, waitingconn) < 0) {
	  printf("connection error (rend)\n");
	}
      } else {
	if (aim_get_command(&aimsess, waitingconn) >= 0) {
	  aim_rxdispatch(&aimsess);
	} else {
	  printf("connection error\n");
	  aim_conn_kill(&aimsess, &waitingconn);
	  if (!aim_getconn_type(&aimsess, AIM_CONN_TYPE_BOS)) {
	    printf("major connetion error\n");
	    keepgoing = 0;
	  }
	}
      }
      break;
      
    default:
      break; /* invalid */
    }
  }

  /* Close up */
  printf("AIM just decided we didn't need to be here anymore, closing up...\n");
  
  /* close up all connections, dead or no */
  aim_logoff(&aimsess); 

  /* Get out */
  exit(0);
}

int faimtest_rateresp(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{

  switch(command->conn->type) {
  case AIM_CONN_TYPE_BOS: {
    /* this is the new buddy list */
    char buddies[] = "Buddy1&Buddy2&ThisHereIsAName2&";
    /* this is the new profile */
    char profile[] = "Hello";  

    aim_bos_ackrateresp(sess, command->conn);  /* ack rate info response */
    aim_bos_reqpersonalinfo(sess, command->conn);
    aim_bos_reqlocaterights(sess, command->conn);
    aim_bos_setprofile(sess, command->conn, profile, NULL, AIM_CAPS_BUDDYICON | AIM_CAPS_CHAT | AIM_CAPS_VOICE | AIM_CAPS_GETFILE | AIM_CAPS_SENDFILE | AIM_CAPS_IMIMAGE);
    aim_bos_reqbuddyrights(sess, command->conn);

    /* send the buddy list and profile (required, even if empty) */
    aim_bos_setbuddylist(sess, command->conn, buddies);

    /* dont really know what this does */
    aim_addicbmparam(sess, command->conn);
    aim_bos_reqicbmparaminfo(sess, command->conn);  
  
    aim_bos_reqrights(sess, command->conn);  
    /* set group permissions -- all user classes */
    aim_bos_setgroupperm(sess, command->conn, AIM_FLAG_ALLUSERS);
    aim_bos_setprivacyflags(sess, command->conn, AIM_PRIVFLAGS_ALLOWIDLE|AIM_PRIVFLAGS_ALLOWMEMBERSINCE);

    break;  
  }

  default: 
    printf("faimtest: got rate response for unhandled connection type %04x\n", command->conn->type);
    break;
  }

  return 1;
}

int faimtest_serverready(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  switch (command->conn->type)
    {
    case AIM_CONN_TYPE_BOS:

      aim_setversions(sess, command->conn);
      aim_bos_reqrate(sess, command->conn); /* request rate info */

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

    case AIM_CONN_TYPE_RENDEZVOUS: /* this is an overloaded function?? - mid */
      aim_conn_addhandler(sess, command->conn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMINCOMING, faimtest_directim_incoming, 0);
      aim_conn_addhandler(sess, command->conn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMDISCONNECT, faimtest_directim_disconnect, 0);
      break;

    default:
      fprintf(stderr, "faimtest: unknown connection type on Server Ready\n");
    }
  return 1;
}

int faimtest_parse_buddyrights(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{	
  va_list ap;
  unsigned short maxbuddies, maxwatchers;

  va_start(ap, command);
  maxbuddies = va_arg(ap, unsigned short);
  maxwatchers = va_arg(ap, unsigned short);
  va_end(ap);

  printf("faimtest: buddy list rights: Max buddies = %d / Max watchers = %d\n", maxbuddies, maxwatchers);

  return 1;
}

int faimtest_bosrights(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  unsigned short maxpermits, maxdenies;
  va_list ap;

  va_start(ap, command);
  maxpermits = va_arg(ap, unsigned short);
  maxdenies = va_arg(ap, unsigned short);
  va_end(ap);

  printf("faimtest: BOS rights: Max permit = %d / Max deny = %d\n", maxpermits, maxdenies);

  aim_bos_clientready(sess, command->conn);

  printf("faimtest: officially connected to BOS.\n");

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
  unsigned char *cookie;

  va_start(ap, command);
  serviceid = va_arg(ap, int);
  ip = va_arg(ap, char *);
  cookie = va_arg(ap, unsigned char *);
 
  switch(serviceid)
    {
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
	if ( (tstconn==NULL) || (tstconn->status >= AIM_CONN_STATUS_RESOLVERR)) {
	  fprintf(stderr, "faimtest: unable to connect to chatnav server\n");
	  if (tstconn) aim_conn_kill(sess, &tstconn);
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
	    if (tstconn) aim_conn_kill(sess, &tstconn);
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
      aim_conn_kill(sess, &command->conn);
      exit(0); /* XXX: should return in order to let the above things get free()'d. */
    }

  printf("Reg status: %2d\n", sess->logininfo.regstatus);
  printf("Email: %s\n", sess->logininfo.email);
  printf("BOS IP: %s\n", sess->logininfo.BOSIP);

  printf("Closing auth connection...\n");
  aim_conn_kill(sess, &command->conn);
  bosconn = aim_newconn(sess, AIM_CONN_TYPE_BOS, sess->logininfo.BOSIP);
  if (bosconn == NULL) {
    fprintf(stderr, "faimtest: could not connect to BOS: internal error\n");
  } else if (bosconn->status != 0) {	
    fprintf(stderr, "faimtest: could not connect to BOS\n");
    aim_conn_kill(sess, &bosconn);
  } else {
    aim_conn_addhandler(sess, bosconn, 0x0009, 0x0003, faimtest_bosrights, 0);
    aim_conn_addhandler(sess, bosconn, 0x0001, 0x0007, faimtest_rateresp, 0); /* rate info */
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_ACK, AIM_CB_ACK_ACK, NULL, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_SERVERREADY, faimtest_serverready, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_RATEINFO, NULL, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_REDIRECT, faimtest_handleredirect, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_STS, AIM_CB_STS_SETREPORTINTERVAL, faimtest_reportinterval, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_RIGHTSINFO, faimtest_parse_buddyrights, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_ONCOMING, faimtest_parse_oncoming, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_OFFGOING, faimtest_parse_offgoing, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_INCOMING, faimtest_parse_incoming_im, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_ERROR, faimtest_parse_locerr, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_MISSEDCALL, faimtest_parse_misses, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_RATECHANGE, faimtest_parse_ratechange, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_EVIL, faimtest_parse_evilnotify, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_ERROR, faimtest_parse_msgerr, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_USERINFO, faimtest_parse_userinfo, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_ACK, faimtest_parse_msgack, 0);

    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_CTN, AIM_CB_CTN_DEFAULT, aim_parse_unknown, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_DEFAULT, aim_parse_unknown, 0);
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_MOTD, faimtest_parse_motd, 0);
    
    aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, faimtest_parse_connerr, 0);
    
    aim_auth_sendcookie(sess, bosconn, sess->logininfo.cookie);
  }
  return 1;
}

static void printuserflags(unsigned short flags)
{
  if (flags & AIM_FLAG_UNCONFIRMED)
    printf("UNCONFIRMED ");
  if (flags & AIM_FLAG_ADMINISTRATOR)
    printf("ADMINISTRATOR ");
  if (flags & AIM_FLAG_AOL)
    printf("AOL ");
  if (flags & AIM_FLAG_OSCAR_PAY)
      printf("OSCAR_PAY ");
  if (flags & AIM_FLAG_FREE)
    printf("FREE ");
  if (flags & AIM_FLAG_AWAY)
    printf("AWAY ");
  if (flags & AIM_FLAG_UNKNOWN40)
    printf("ICQ? ");
  if (flags & AIM_FLAG_UNKNOWN80)
    printf("UNKNOWN80 ");
  return;
}

int faimtest_parse_userinfo(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  struct aim_userinfo_s *userinfo;
  char *prof_encoding = NULL;
  char *prof = NULL;
  unsigned short inforeq = 0;

  va_list ap;
  va_start(ap, command);
  userinfo = va_arg(ap, struct aim_userinfo_s *);
  prof_encoding = va_arg(ap, char *);
  prof = va_arg(ap, char *);
  inforeq = va_arg(ap, unsigned short);
  va_end(ap);
  
  printf("faimtest: userinfo: sn: %s\n", userinfo->sn);
  printf("faimtest: userinfo: warnlevel: 0x%04x\n", userinfo->warnlevel);
  printf("faimtest: userinfo: flags: 0x%04x = ", userinfo->flags);
  printuserflags(userinfo->flags);
  printf("\n");
  
  printf("faimtest: userinfo: membersince: %lu\n", userinfo->membersince);
  printf("faimtest: userinfo: onlinesince: %lu\n", userinfo->onlinesince);
  printf("faimtest: userinfo: idletime: 0x%04x\n", userinfo->idletime);
  
  if (inforeq == AIM_GETINFO_GENERALINFO) {
    printf("faimtest: userinfo: profile_encoding: %s\n", prof_encoding ? prof_encoding : "[none]");
    printf("faimtest: userinfo: prof: %s\n", prof ? prof : "[none]");
  } else if (inforeq == AIM_GETINFO_AWAYMESSAGE) {
    printf("faimtest: userinfo: awaymsg_encoding: %s\n", prof_encoding ? prof_encoding : "[none]");
    printf("faimtest: userinfo: awaymsg: %s\n", prof ? prof : "[none]");
  } else 
    printf("faimtest: userinfo: unknown info request\n");
  
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
 *  int                         flags       flags
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
  if (channel == 1) {
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
    printf("faimtest: icbm: flags = 0x%04x = ", userinfo->flags);
    printuserflags(userinfo->flags);
    printf("\n");

    printf("faimtest: icbm: membersince = %lu\n", userinfo->membersince);
    printf("faimtest: icbm: onlinesince = %lu\n", userinfo->onlinesince);
    printf("faimtest: icbm: idletime = 0x%04x\n", userinfo->idletime);
    printf("faimtest: icbm: capabilities = 0x%04x\n", userinfo->capabilities);
    
    printf("faimtest: icbm: icbmflags = ");
    if (icbmflags & AIM_IMFLAGS_AWAY)
      printf("away ");
    if (icbmflags & AIM_IMFLAGS_ACK)
      printf("ackrequest ");
    printf("\n");
    
    printf("faimtest: icbm: encoding flags = {%04x, %04x}\n", flag1, flag2);
    
    printf("faimtest: icbm: message: %s\n", msg);
    
    if (msg) {
      int i = 0;

      while (msg[i] == '<') {
	if (msg[i] == '<') {
	  while (msg[i] != '>')
	    i++;
	  i++;
	}
      }
      tmpstr = msg+i;

      printf("tmpstr = %s\n", tmpstr);
      
      if ( (strlen(tmpstr) >= 10) &&
	   (!strncmp(tmpstr, "disconnect", 10)) ) {
	  aim_send_im(sess, command->conn, "midendian", 0, "ta ta...");
	  aim_logoff(sess);
      } else if (strstr(tmpstr, "goodday")) {
	printf("faimtest: icbm: sending response\n");
	aim_send_im(sess, command->conn, userinfo->sn, AIM_IMFLAGS_ACK, "Good day to you too.");
      } else if (strstr(tmpstr, "warnme")) {
	printf("faimtest: icbm: sending non-anon warning\n");
	aim_send_warning(sess, command->conn, userinfo->sn, 0);
      } else if (strstr(tmpstr, "anonwarn")) {
	printf("faimtest: icbm: sending anon warning\n");
	aim_send_warning(sess, command->conn, userinfo->sn, AIM_WARN_ANON);
      } else if (!strncmp(tmpstr, "open chatnav", 12)) {
	aim_bos_reqservice(sess, command->conn, AIM_CONN_TYPE_CHATNAV);
	//aim_chat_join(sess, command->conn, "thishereisaname2_chat85");
      } else if (!strncmp(tmpstr, "create", 6)) {
	aim_chatnav_createroom(sess,aim_getconn_type(sess, AIM_CONN_TYPE_CHATNAV), (strlen(tmpstr) < 7)?"WorldDomination":tmpstr+7, 0x0004);
      } else if (!strncmp(tmpstr, "close chatnav", 13)) {
	struct aim_conn_t *chatnavconn;
	chatnavconn = aim_getconn_type(sess, AIM_CONN_TYPE_CHATNAV);
	aim_conn_kill(sess, &chatnavconn);
      } else if (!strncmp(tmpstr, "join", 4)) {
	  aim_chat_join(sess, command->conn, 0x0004, "worlddomination");
      } else if (!strncmp(tmpstr, "leave", 5))
	    aim_chat_leaveroom(sess, "worlddomination");
      else if (!strncmp(tmpstr, "getinfo", 7)) {
	aim_getinfo(sess, command->conn, "75784102", AIM_GETINFO_GENERALINFO);
	aim_getinfo(sess, command->conn, "15853637", AIM_GETINFO_AWAYMESSAGE);
      } else if (!strncmp(tmpstr, "open directim", 13)) {
	struct aim_conn_t *newconn;
	newconn = aim_directim_initiate(sess, command->conn, NULL, userinfo->sn);
	//aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMINITIATE, faimtest_directim_initiate, 0);
      } else if (!strncmp(tmpstr, "reqsendmsg", 10)) {
	aim_send_im(sess, command->conn, "vaxherder", 0, "sendmsg 7900");
      } else if (!strncmp(tmpstr, "sendmsg", 7)) {
	int i;
	i = atoi(tmpstr+8);
	if (i < 10000) {
	  char *newbuf;
	  int z;

	  newbuf = malloc(i+1);
	  for (z = 0; z < i; z++) {
	    newbuf[z] = (z % 10)+0x30;
	  }
	  newbuf[i] = '\0';
	  aim_send_im(sess, command->conn, userinfo->sn, 0, newbuf);
	  free(newbuf);
	}
      } else {
	printf("unknown command.\n");
	aim_add_buddy(sess, command->conn, userinfo->sn);
      }
      
    }
  }
  /*
   * Channel 2: Rendevous Request
   */
  else if (channel == 2) {
    struct aim_userinfo_s *userinfo;
    unsigned short reqclass;
    
    reqclass = va_arg(ap, unsigned short);
    switch (reqclass) {
    case AIM_CAPS_VOICE: {
      userinfo = va_arg(ap, struct aim_userinfo_s *);
      va_end(ap);
      
      printf("faimtest: voice invitation: source sn = %s\n", userinfo->sn);
      printf("faimtest: voice invitation: \twarnlevel = 0x%04x\n", userinfo->warnlevel);
      printf("faimtest: voice invitation: \tclass = 0x%04x = ", userinfo->flags);
      printuserflags(userinfo->flags);
      printf("\n");

      /* we dont get membersince on chat invites! */
      printf("faimtest: voice invitation: \tonlinesince = %lu\n", userinfo->onlinesince);
      printf("faimtest: voice invitation: \tidletime = 0x%04x\n", userinfo->idletime);
      
      break;
    }
    case AIM_CAPS_GETFILE: {
      printf("faimtset: get file!\n");
      break;
    }
    case AIM_CAPS_SENDFILE: {
      printf("faimtest: send file!\n");
      break;
    }
    case AIM_CAPS_CHAT: {
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
      printf("faimtest: chat invitation: \tclass = 0x%04x = ", userinfo->flags);
      printuserflags(userinfo->flags);
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
      break;
    }	
    case AIM_CAPS_IMIMAGE: {
      struct aim_directim_priv *priv;
      struct aim_conn_t *newconn;

      printf("faimtest: icbm: rendezvous imimage\n");
     
      userinfo = va_arg(ap, struct aim_userinfo_s *);
      priv = va_arg(ap, struct aim_directim_priv *);
      va_end(ap);

      printf("faimtest: OFT: DirectIM: request from %s (%s)\n", userinfo->sn, priv->ip);
      
      if (!(newconn = aim_directim_connect(sess, command->conn, priv))) {
	printf("faimtest: icbm: imimage: could not connect\n");
	break;
      }
      aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMINCOMING, faimtest_directim_incoming, 0);
      aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMDISCONNECT, faimtest_directim_disconnect, 0);
      aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMTYPING, faimtest_directim_typing, 0);

      aim_send_im_direct(sess, newconn, "goodday");

      printf("faimtest: OFT: DirectIM: connected to %s\n", userinfo->sn);

      break;
    }
    default:
      printf("faimtest: icbm: unknown reqclass (%d)\n", reqclass);
    } /* switch */
  } else
    printf("faimtest does not support channels > 2 (chan = %02x)\n", channel);
  printf("faimtest: icbm: done with ICBM handling\n");

  return 1;
}

#if 0
int faimtest_directim_initiate(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  struct aim_directim_priv *priv;
  struct aim_conn_t *newconn;

  ap = va_start(ap, command);
  newconn = va_arg(ap, struct aim_conn_t *);
  va_end(ap);

  priv = (struct aim_directim_priv *)newconn->priv;

  printf("faimtest: OFT: DirectIM: intitiate success to %s\n", priv->ip);
  
  aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMINCOMING, faimtest_directim_incoming, 0);
  aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMDISCONNECT, faimtest_directim_disconnect, 0);
  aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMTYPING, faimtest_directim_typing, 0);

  aim_send_im_direct(sess, newconn, "goodday");

  printf("faimtest: OFT: DirectIM: connected to %s\n", priv->sn);

  return 1;
}
#endif

int faimtest_directim_connect(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  struct aim_directim_priv *priv;
  
  ap = va_start(ap, command);
  priv = va_arg(ap, struct aim_directim_priv *);

  va_end(ap);
  
  printf("faimtest: directim_connect\n");

  return 1;
}

int faimtest_directim_incoming(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  char *sn = NULL, *msg = NULL;
  struct aim_conn_t *conn;

  ap = va_start(ap, command);
  conn = va_arg(ap, struct aim_conn_t *);
  sn = va_arg(ap, char *);
  msg = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: Directim from %s: %s\n", sn, msg);
  if (!strncmp(msg, "sendmsg", 7)) {
    int i;
    i = atoi(msg+8);
    if (i < 10000) {
      char *newbuf;
      int z;
      
      newbuf = malloc(i+1);
      for (z = 0; z < i; z++) {
	newbuf[z] = (z % 10)+0x30;
      }
      newbuf[i] = '\0';
      aim_send_im_direct(sess, conn, newbuf);
      free(newbuf);
    }
  } else if (!strncmp(msg, "goodday", 7)) {
    aim_send_im_direct(sess, conn, "Good day to you, too");
  } else {
    char newmsg[1024];
    snprintf(newmsg, sizeof(newmsg), "unknown (%s)\n", msg);
    aim_send_im_direct(sess, conn, newmsg);
  }
  return 1;
}

int faimtest_directim_disconnect(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  printf("faimtest: directim_disconnect\n");
  return 1;
}

int faimtest_directim_typing(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  char *sn;
  
  ap = va_start(ap, command);
  sn = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: ohmigod! %s has started typing (DirectIM). He's going to send you a message! *squeal*\n", sn);
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

  printf("\n%s is now online (flags: %04x = %s%s%s%s%s%s%s%s) (caps = 0x%04x)\n",
	 userinfo->sn, userinfo->flags,
	 (userinfo->flags&AIM_FLAG_UNCONFIRMED)?" UNCONFIRMED":"",
	 (userinfo->flags&AIM_FLAG_ADMINISTRATOR)?" ADMINISTRATOR":"",
	 (userinfo->flags&AIM_FLAG_AOL)?" AOL":"",
	 (userinfo->flags&AIM_FLAG_OSCAR_PAY)?" OSCAR_PAY":"",
	 (userinfo->flags&AIM_FLAG_FREE)?" FREE":"",
	 (userinfo->flags&AIM_FLAG_AWAY)?" AWAY":"",
	 (userinfo->flags&AIM_FLAG_UNKNOWN40)?" UNKNOWN40":"",
	 (userinfo->flags&AIM_FLAG_UNKNOWN80)?" UNKNOWN80":"",
	 userinfo->capabilities);
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
  static char *codes[] = {
    "Unknown",
    "Mandatory upgrade",
    "Advisory upgrade",
    "System bulletin",
    "Top o' the world!"};
  static int codeslen = 5;

  char *msg;
  unsigned short id;
  va_list ap;
  
  va_start(ap, command);
  id = va_arg(ap, unsigned short);
  msg = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: motd: %s (%d / %s)\n", msg, id, 
	 (id < codeslen)?codes[id]:"unknown");

  return 1;
}

int faimtest_parse_msgerr(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  char *destsn;
  unsigned short reason;

  va_start(ap, command);
  destsn = va_arg(ap, char *);
  reason = va_arg(ap, unsigned short);
  va_end(ap);

  printf("faimtest: message to %s bounced (reason 0x%04x: %s)\n", destsn, reason, (reason<msgerrreasonslen)?msgerrreasons[reason]:"unknown");
  
  return 1;
}

int faimtest_parse_locerr(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  char *destsn;
  unsigned short reason;

  va_start(ap, command);
  destsn = va_arg(ap, char *);
  reason = va_arg(ap, unsigned short);
  va_end(ap);

  printf("faimtest: user information for %s unavailable (reason 0x%04x: %s)\n", destsn, reason, (reason<msgerrreasonslen)?msgerrreasons[reason]:"unknown");
  
  return 1;
}

/* 
 * Handles callbacks for AIM_CB_MISSED_CALL.
 */
int faimtest_parse_misses(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  static char *missedreasons[] = {
    "Unknown",
    "Message too large"};
  static int missedreasonslen = 2;

  va_list ap;
  unsigned short chan, nummissed, reason;
  struct aim_userinfo_s *userinfo;
  
  va_start(ap, command);
  chan = va_arg(ap, unsigned short);
  userinfo = va_arg(ap, struct aim_userinfo_s *);
  nummissed = va_arg(ap, unsigned short);
  reason = va_arg(ap, unsigned short);
  va_end(ap);

  printf("faimtest: missed %d messages from %s (reason %d: %s)\n", nummissed, userinfo->sn, reason, (reason<missedreasonslen)?missedreasons[reason]:"unknown");
  
  return 0;
}

int faimtest_parse_login(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  struct client_info_s info = {"faimtest (with SNAC login)", 4, 1, 2010, "us", "en", 0x0004, 0x0000, 0x0000004b}; /* 4.1.2010 */
  char *key;
  va_list ap;
  
  va_start(ap, command);
  key = va_arg(ap, char *);
  va_end(ap);

  aim_send_login(sess, command->conn, screenname, password, &info, key);
 
  return 1;
}

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
  unsigned short unknown_c9, unknown_d2, unknown_d5, maxmsglen;
  unsigned long creationtime;

  va_start(ap, command);
  roominfo = va_arg(ap, struct aim_chat_roominfo *);
  roomname = va_arg(ap, char *);
  usercount= va_arg(ap, int);
  userinfo = va_arg(ap, struct aim_userinfo_s *);
  roomdesc = va_arg(ap, char *);
  unknown_c9 = va_arg(ap, unsigned short);
  creationtime = va_arg(ap, unsigned long);
  maxmsglen = va_arg(ap, unsigned short);
  unknown_d2 = va_arg(ap, unsigned short);
  unknown_d5 = va_arg(ap, unsigned short);
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

  printf("faimtest: chat: %s:  \tUnknown_c9: 0x%04x\n", (char *)command->conn->priv, unknown_c9);
  printf("faimtest: chat: %s:  \tCreation time: %lu (time_t)\n", (char *)command->conn->priv, creationtime);
  printf("faimtest: chat: %s:  \tMax message length: %d bytes\n", (char *)command->conn->priv, maxmsglen);
  printf("faimtest: chat: %s:  \tUnknown_d2: 0x%04x\n", (char *)command->conn->priv, unknown_d2);
  printf("faimtest: chat: %s:  \tUnknown_d5: 0x%02x\n", (char *)command->conn->priv, unknown_d5);

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

int faimtest_parse_connerr(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  unsigned short code;
  char *msg = NULL;

  ap = va_start(ap, command);
  code = va_arg(ap, unsigned short);
  msg = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: connerr: Code 0x%04x: %s\n", code, msg);
  aim_conn_kill(sess, &command->conn); /* this will break the main loop */

  return 1;
}

int faimtest_debugconn_connect(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{	
  printf("faimtest: connecting to an aimdebugd!\n");

  /* convert the authorizer connection to a BOS connection */
  command->conn->type = AIM_CONN_TYPE_BOS;

  aim_conn_addhandler(sess, command->conn, AIM_CB_FAM_MSG, AIM_CB_MSG_INCOMING, faimtest_parse_incoming_im, 0);

  /* tell the aimddebugd we're ready */
  aim_debugconn_sendconnect(sess, command->conn); 

  /* go right into main loop (don't open a BOS connection, etc) */
  return 1;
}

/*
 * Recieved in response to an IM sent with the AIM_IMFLAGS_ACK option.
 */
int faimtest_parse_msgack(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  unsigned short type;
  char *sn = NULL;

  ap = va_start(ap, command);
  type = va_arg(ap, unsigned short);
  sn = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: msgack: 0x%04x / %s\n", type, sn);

  return 1;
}

int faimtest_parse_ratechange(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  unsigned long newrate;
  
  va_start(ap, command); 
  newrate = va_arg(ap, unsigned long);
  va_end(ap);

  printf("faimtest: ratechange: %lu\n", newrate);

  return (1);
}

int faimtest_parse_evilnotify(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  char *sn;

  va_start(ap, command);
  sn = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: warning from: %s\n", sn);

  return 1;
}
