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
int faimtest_hostversions(struct aim_session_t *sess, struct command_rx_struct *command, ...);
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
#define FILESUPPORT
#ifdef FILESUPPORT
int faimtest_getfile_filereq(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_getfile_filesend(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_getfile_complete(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_getfile_disconnect(struct aim_session_t *sess, struct command_rx_struct *command, ...);
#endif

int faimtest_parse_ratechange(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_parse_evilnotify(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_parse_msgerr(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_parse_buddyrights(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_parse_locerr(struct aim_session_t *sess, struct command_rx_struct *command, ...);
int faimtest_parse_genericerr(struct aim_session_t *sess, struct command_rx_struct *command, ...);

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

static char *screenname,*password,*server=NULL;
static int connected = 0;

int faimtest_reportinterval(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  if (command->data) {
    printf("aim: minimum report interval: %d (seconds?)\n", aimutil_get16(command->data+10));
  } else
    printf("aim: NULL minimum report interval!\n");
  return 1;
}

int faimtest_flapversion(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{

  printf("faimtest: using FLAP version %u\n", aimutil_get32(command->data));

#if 0
  /* 
   * This is an alternate location for starting the login process.
   */
  /* XXX should do more checking to make sure its really the right AUTH conn */
  if (command->conn->type == AIM_CONN_TYPE_AUTH) {
    /* do NOT send a connack/flapversion, request_login will send it if needed */
    aim_request_login(sess, command->conn, screenname);
    printf("faimtest: login request sent\n");
  }
#endif

  return 1;
}

/*
 * This is a frivilous callback. You don't need it. I only used it for
 * debugging non-blocking connects.
 *
 * If packets are sent to a conn before its fully connected, they
 * will be queued and then transmitted when the connection completes.
 *
 */
int faimtest_conncomplete(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  struct aim_conn_t *conn;

  va_start(ap, command);
  conn = va_arg(ap, struct aim_conn_t *);
  va_end(ap);
  
  if (conn)
    printf("faimtest: connection on %d completed\n", conn->fd);

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

int main(void)
{
  struct aim_session_t aimsess;
  struct aim_conn_t *authconn = NULL, *waitingconn = NULL;
  int keepgoing = 1;
  char *proxy, *proxyusername, *proxypass;

  char *listingpath;

  int selstat = 0;

#ifdef FILESUPPORT
  FILE *listingfile;
#endif

  if ( !(screenname = getenv("SCREENNAME")) ||
       !(password = getenv("PASSWORD")))
    {
      printf("Must specify SCREENAME and PASSWORD in environment.\n");
      return -1;
    }

  server = getenv("AUTHSERVER");

  proxy = getenv("SOCKSPROXY");
  proxyusername = getenv("SOCKSNAME");
  proxypass = getenv("SOCKSPASS");

#ifdef FILESUPPORT
  listingpath = getenv("LISTINGPATH");
#endif

#ifdef _WIN32
  if (initwsa() != 0) {
    printf("faimtest: could not initialize windows sockets\n");
    return -1;
  }
#endif /* _WIN32 */

  /* Pass zero as flags if you want blocking connects */
  aim_session_init(&aimsess, AIM_SESS_FLAGS_NONBLOCKCONNECT);

#ifdef FILESUPPORT
  if(listingpath) {
    char *listingname;
    if(!(listingname = (char *)calloc(1, strlen(listingpath)+strlen("/listing.txt")))) {
      perror("listingname calloc.");
      exit(-1);
    }
    sprintf(listingname, "%s/listing.txt", listingpath);
    if( (listingfile = fopen(listingname, "r")) == NULL) {
      printf("Couldn't open %s... bombing.\n", listingname);
      exit(-1);
    }

    aim_oft_registerlisting(&aimsess, listingfile, listingpath);

    free(listingname);
  }
#endif

  if (proxy)
    aim_setupproxy(&aimsess, proxy, proxyusername, proxypass);

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

  aim_conn_addhandler(&aimsess, authconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_FLAPVER, faimtest_flapversion, 0);
  aim_conn_addhandler(&aimsess, authconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
  aim_conn_addhandler(&aimsess, authconn, 0x0017, 0x0007, faimtest_parse_login, 0);
  aim_conn_addhandler(&aimsess, authconn, 0x0017, 0x0003, faimtest_parse_authresp, 0);    

  aim_conn_addhandler(&aimsess, authconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_DEBUGCONN_CONNECT, faimtest_debugconn_connect, 0);

  /* If the connection is in progress, this will just be queued */
  aim_request_login(&aimsess, authconn, screenname);
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
	  printf("connection error (rend out)\n");
	}
      } else {
	if (aim_get_command(&aimsess, waitingconn) >= 0) {
	  aim_rxdispatch(&aimsess);
	} else {
	  printf("connection error (type 0x%04x:0x%04x)\n", waitingconn->type, waitingconn->subtype);
	  if(waitingconn->type == AIM_CONN_TYPE_RENDEZVOUS) {
	    /* we should have callbacks for all these, else the library will do the conn_kill for us. */
	    printf("connection error: rendezvous connection. you forgot register a disconnect callback, right?\n");
	  }
	  else
	    aim_conn_kill(&aimsess, &waitingconn);
	  if (!aim_getconn_type(&aimsess, AIM_CONN_TYPE_BOS)) {
	    printf("major connection error\n");
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
    char buddies[] = "Buddy1&Buddy2&ThisHereIsAName2&midendian&ewarmenhoven&";
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

int faimtest_hostversions(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  int vercount, i;
  unsigned char *versions;
  va_list ap;

  va_start(ap, command);
  vercount = va_arg(ap, int); /* number of family/version pairs */
  versions = va_arg(ap, unsigned char *);
  va_end(ap);

  printf("faimtest: SNAC versions supported by this host: ");
  for (i = 0; i < vercount*4; i += 4)
    printf("0x%04x:0x%04x ", 
	   aimutil_get16(versions+i),  /* SNAC family */
	   aimutil_get16(versions+i+2) /* Version number */);
  printf("\n");

  return 1;
}

int faimtest_serverready(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  int famcount, i;
  unsigned short *families;
  va_list ap;

  va_start(ap, command);
  famcount = va_arg(ap, int);
  families = va_arg(ap, unsigned short *);
  va_end(ap);

  printf("faimtest: SNAC families supported by this host (type %d): ", command->conn->type);
  for (i = 0; i < famcount; i++)
    printf("0x%04x ", families[i]);
  printf("\n");

  switch (command->conn->type) {
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

  case AIM_CONN_TYPE_RENDEZVOUS: /* empty */
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
  maxbuddies = va_arg(ap, int);
  maxwatchers = va_arg(ap, int);
  va_end(ap);

  printf("faimtest: buddy list rights: Max buddies = %d / Max watchers = %d\n", maxbuddies, maxwatchers);

  return 1;
}

int faimtest_bosrights(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  unsigned short maxpermits, maxdenies;
  va_list ap;

  va_start(ap, command);
  maxpermits = va_arg(ap, int);
  maxdenies = va_arg(ap, int);
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
	if ( (tstconn==NULL) || (tstconn->status & AIM_CONN_STATUS_RESOLVERR) )
	  fprintf(stderr, "faimtest: unable to reconnect with authorizer\n");
	else {
	  aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_FLAPVER, faimtest_flapversion, 0);
	  aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
	  /* Send the cookie to the Auth */
	  aim_auth_sendcookie(sess, tstconn, cookie);
	}

      }  
      break;
    case 0x000d: /* ChatNav */
      {
	struct aim_conn_t *tstconn = NULL;
	tstconn = aim_newconn(sess, AIM_CONN_TYPE_CHATNAV, ip);
	if ( (tstconn==NULL) || (tstconn->status & AIM_CONN_STATUS_RESOLVERR)) {
	  fprintf(stderr, "faimtest: unable to connect to chatnav server\n");
	  if (tstconn) aim_conn_kill(sess, &tstconn);
	  return 1;
	}
#if 0
	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_CTN, AIM_CB_SPECIAL_DEFAULT, aim_parse_unknown, 0);
	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_GEN, AIM_CB_SPECIAL_DEFAULT, aim_parse_unknown, 0);
#endif
	aim_conn_addhandler(sess, tstconn, 0x0001, 0x0003, faimtest_serverready, 0);
	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
	aim_auth_sendcookie(sess, tstconn, cookie);
	fprintf(stderr, "\achatnav: connected\n");
      }
      break;
    case 0x000e: /* Chat */
      {
	char *roomname = NULL;
	int exchange;
	struct aim_conn_t *tstconn = NULL;

	roomname = va_arg(ap, char *);
	exchange = va_arg(ap, int);

	tstconn = aim_newconn(sess, AIM_CONN_TYPE_CHAT, ip);
	if ( (tstconn==NULL) || (tstconn->status & AIM_CONN_STATUS_RESOLVERR))
	  {
	    fprintf(stderr, "faimtest: unable to connect to chat server\n");
	    if (tstconn) aim_conn_kill(sess, &tstconn);
	    return 1;
	  }		
	printf("faimtest: chat: connected to %s on exchange %d\n", roomname, exchange);

	/*
	 * We must do this to attach the stored name to the connection!
	 */
	aim_chat_attachname(tstconn, roomname);

	aim_conn_addhandler(sess, tstconn, 0x0001, 0x0003, faimtest_serverready, 0);
	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
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
  va_list ap;
  struct aim_conn_t *bosconn = NULL;
  char *sn = NULL, *bosip = NULL, *errurl = NULL, *email = NULL;
  unsigned char *cookie = NULL;
  int errorcode = 0, regstatus = 0;
  int latestbuild = 0, latestbetabuild = 0;
  char *latestrelease = NULL, *latestbeta = NULL;
  char *latestreleaseurl = NULL, *latestbetaurl = NULL;
  char *latestreleaseinfo = NULL, *latestbetainfo = NULL;

  va_start(ap, command);
  sn = va_arg(ap, char *);
  errorcode = va_arg(ap, int);
  errurl = va_arg(ap, char *);
  regstatus = va_arg(ap, int);
  email = va_arg(ap, char *);
  bosip = va_arg(ap, char *);
  cookie = va_arg(ap, unsigned char *);

  latestrelease = va_arg(ap, char *);
  latestbuild = va_arg(ap, int);
  latestreleaseurl = va_arg(ap, char *);
  latestreleaseinfo = va_arg(ap, char *);

  latestbeta = va_arg(ap, char *);
  latestbetabuild = va_arg(ap, int);
  latestbetaurl = va_arg(ap, char *);
  latestbetainfo = va_arg(ap, char *);

  va_end(ap);

  printf("Screen name: %s\n", sn);

  /*
   * Check for error.
   */
  if (errorcode || !bosip || !cookie) {
    printf("Login Error Code 0x%04x\n", errorcode);
    printf("Error URL: %s\n", errurl);
    aim_conn_kill(sess, &command->conn); 
    return 1;
  }

  printf("Reg status: %2d\n", regstatus);
  printf("Email: %s\n", email);
  printf("BOS IP: %s\n", bosip);

  if (latestbeta)
    printf("Latest beta version: %s, build %d, at %s (more info at %s)\n", latestbeta, latestbetabuild, latestbetaurl, latestbetainfo);

  if (latestrelease)
    printf("Latest released version: %s, build %d, at %s (more info at %s)\n", latestrelease, latestbuild, latestreleaseurl, latestreleaseinfo);

  printf("Closing auth connection...\n");
  aim_conn_kill(sess, &command->conn);
  if (!(bosconn = aim_newconn(sess, AIM_CONN_TYPE_BOS, bosip))) {
    fprintf(stderr, "faimtest: could not connect to BOS: internal error\n");
    return 1;
  } else if (bosconn->status & AIM_CONN_STATUS_CONNERR) {	
    fprintf(stderr, "faimtest: could not connect to BOS\n");
    aim_conn_kill(sess, &bosconn);
    return 1;
  }

  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
  aim_conn_addhandler(sess, bosconn, 0x0009, 0x0003, faimtest_bosrights, 0);
  aim_conn_addhandler(sess, bosconn, 0x0001, 0x0007, faimtest_rateresp, 0); /* rate info */
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_ACK, AIM_CB_ACK_ACK, NULL, 0);
  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, 0x0018, faimtest_hostversions, 0);
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
    
  aim_conn_addhandler(sess, bosconn, 0x0001, 0x0001, faimtest_parse_genericerr, 0);
  aim_conn_addhandler(sess, bosconn, 0x0003, 0x0001, faimtest_parse_genericerr, 0);
  aim_conn_addhandler(sess, bosconn, 0x0009, 0x0001, faimtest_parse_genericerr, 0);

  aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, faimtest_parse_connerr, 0);


  aim_auth_sendcookie(sess, bosconn, cookie);

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
  inforeq = va_arg(ap, int);
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
    unsigned short flag1, flag2;
    
    userinfo = va_arg(ap, struct aim_userinfo_s *);
    msg = va_arg(ap, char *);
    icbmflags = va_arg(ap, u_int);
    flag1 = va_arg(ap, int);
    flag2 = va_arg(ap, int);
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
      } else if (strstr(tmpstr, "setdirectoryinfo")) {
	printf("faimtest: icbm: sending backwards profile data\n");
	aim_setdirectoryinfo(sess, command->conn, "tsrif", "elddim", "tsal", "nediam", "emankcin", "teerts", "ytic", "etats", "piz", 0, 1);
      } else if (strstr(tmpstr, "setinterests")) {
	printf("faimtest: icbm: setting fun interests\n");
	aim_setuserinterests(sess, command->conn, "interest1", "interest2", "interest3", "interest4", "interest5", 1);
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
      } else if (!strncmp(tmpstr, "openauth", 8)) {
	aim_bos_reqservice(sess, command->conn, AIM_CONN_TYPE_AUTH);
      } else if (!strncmp(tmpstr, "auth", 4)) {
	aim_genericreq_n(sess, aim_getconn_type(sess, AIM_CONN_TYPE_AUTH), 0x0007, 0x0002);
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
    
    reqclass = va_arg(ap, int);
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
#ifdef FILESUPPORT
      char *ip, *cookie;
      struct aim_conn_t *newconn;

      userinfo = va_arg(ap, struct aim_userinfo_s *);
      ip = va_arg(ap, char *);
      cookie = va_arg(ap, char *);
      va_end(ap);
      
      printf("faimtest: get file request from %s (at %s)\n", userinfo->sn, ip);

      sleep(1);

      if( (newconn = aim_accepttransfer(sess, command->conn, userinfo->sn, cookie, ip, sess->oft.listing, reqclass)) == NULL ) {
	printf("faimtest: getfile: requestconn: apparent error in accepttransfer\n");
	break;
      }
      
      aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEFILEREQ,  faimtest_getfile_filereq, 0);
      aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEFILESEND, faimtest_getfile_filesend, 0);
      aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILECOMPLETE, faimtest_getfile_complete, 0);      
      aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEDISCONNECT, faimtest_getfile_disconnect, 0);      

      printf("faimtest: getfile connect succeeded, handlers added.\n");

      break;
#endif
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

      printf("faimtest: OFT: DirectIM: connected to %s\n", userinfo->sn);

      aim_send_im_direct(sess, newconn, "goodday");

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

int faimtest_directim_initiate(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  struct aim_directim_priv *priv;
  struct aim_conn_t *newconn;

  va_start(ap, command);
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

int faimtest_directim_connect(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  struct aim_directim_priv *priv;
  
  va_start(ap, command);
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

  va_start(ap, command);
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
  va_list ap;
  struct aim_conn_t *conn;
  char *sn;

  va_start(ap, command);
  conn = va_arg(ap, struct aim_conn_t *);
  sn = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: directim: disconnected from %s\n", sn);

  aim_conn_kill(sess, &conn);
  return 1;
}

int faimtest_directim_typing(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  char *sn;
  
  va_start(ap, command);
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

  printf("%ld  %s is now online (flags: %04x = %s%s%s%s%s%s%s%s) (caps = 0x%04x)\n",
	 time(NULL),
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
  id = va_arg(ap, int);
  msg = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: motd: %s (%d / %s)\n", msg, id, 
	 (id < codeslen)?codes[id]:"unknown");

  if (!connected)
    connected++;

  return 1;
}

int faimtest_parse_genericerr(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  unsigned short reason;

  va_start(ap, command);
  reason = va_arg(ap, int);
  va_end(ap);

  printf("faimtest: snac threw error (reason 0x%04x: %s)\n", reason, (reason<msgerrreasonslen)?msgerrreasons[reason]:"unknown");
  
  return 1;
}

int faimtest_parse_msgerr(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  char *destsn;
  unsigned short reason;

  va_start(ap, command);
  destsn = va_arg(ap, char *);
  reason = va_arg(ap, int);
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
  reason = va_arg(ap, int);
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
  chan = va_arg(ap, int);
  userinfo = va_arg(ap, struct aim_userinfo_s *);
  nummissed = va_arg(ap, int);
  reason = va_arg(ap, int);
  va_end(ap);

  printf("faimtest: missed %d messages from %s (reason %d: %s)\n", nummissed, userinfo->sn, reason, (reason<missedreasonslen)?missedreasons[reason]:"unknown");
  
  return 1;
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
  unknown_c9 = va_arg(ap, int);
  creationtime = va_arg(ap, unsigned long);
  maxmsglen = va_arg(ap, int);
  unknown_d2 = va_arg(ap, int);
  unknown_d5 = va_arg(ap, int);
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
  if (strcmp(userinfo->sn, sess->sn) != 0)
    {
      sprintf(tmpbuf, "(%s said \"%s\")", userinfo->sn, msg);
      aim_chat_send_im(sess, command->conn, tmpbuf);
    }

  return 1;
}

int faimtest_chatnav_info(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  unsigned short type;
  va_list ap;

  va_start(ap, command);
  type = va_arg(ap, int);

  switch(type) {
  case 0x0002: {
    int maxrooms;
    struct aim_chat_exchangeinfo *exchanges;
    int exchangecount,i = 0;
    
    maxrooms = va_arg(ap, int);
    exchangecount = va_arg(ap, int);
    exchanges = va_arg(ap, struct aim_chat_exchangeinfo *);
    va_end(ap);
    
    printf("faimtest: chat info: Chat Rights:\n");
    printf("faimtest: chat info: \tMax Concurrent Rooms: %d\n", maxrooms);
    
    printf("faimtest: chat info: \tExchange List: (%d total)\n", exchangecount);
    while (i < exchangecount) {
      printf("faimtest: chat info: \t\t%x: %s (%s/%s)\n", 
	     exchanges[i].number,	
	     exchanges[i].name,
	     exchanges[i].charset1,
	     exchanges[i].lang1);
      i++;
    }
    
  }
  break;
  case 0x0008: {
    char *fqcn, *name, *ck;
    unsigned short instance, flags, maxmsglen, maxoccupancy, unknown, exchange;
    unsigned char createperms;
    unsigned long createtime;

    fqcn = va_arg(ap, char *);
    instance = va_arg(ap, int);
    exchange = va_arg(ap, int);
    flags = va_arg(ap, int);
    createtime = va_arg(ap, unsigned long);
    maxmsglen = va_arg(ap, int);
    maxoccupancy = va_arg(ap, int);
    createperms = va_arg(ap, int);
    unknown = va_arg(ap, int);
    name = va_arg(ap, char *);
    ck = va_arg(ap, char *);
    va_end(ap);

    printf("faimtest: recieved room create reply for %s/0x%04x\n", fqcn, exchange);
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

  va_start(ap, command);
  code = va_arg(ap, int);
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

  va_start(ap, command);
  type = va_arg(ap, int);
  sn = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: msgack: 0x%04x / %s\n", type, sn);

  return 1;
}

#ifdef FILESUPPORT
int faimtest_getfile_filereq(struct aim_session_t *ses, struct command_rx_struct *command, ...)
{
  va_list ap;
  struct aim_conn_t *oftconn;
  struct aim_fileheader_t *fh;
  char *cookie;

  va_start(ap, command);
  oftconn = va_arg(ap, struct aim_conn_t *);
  fh = va_arg(ap, struct aim_fileheader_t *);
  cookie = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: request for file %s.\n", fh->name);

  return 1;
}


int faimtest_getfile_filesend(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  struct aim_conn_t *oftconn;
  struct aim_fileheader_t *fh;
  char *path, *cookie;

  FILE *file;

  va_start(ap, command);
  oftconn = va_arg(ap, struct aim_conn_t *);
  fh = va_arg(ap, struct aim_fileheader_t *);
  cookie = va_arg(ap, char *);
  va_end(ap);

  printf("faimtest: sending file %s.\n", fh->name);

  if( (path = (char *)calloc(1, strlen(sess->oft.listingdir) +strlen(fh->name)+2)) == NULL) {
    perror("calloc:");
    printf("faimtest: error in calloc of path\n");
    return 0; /* XXX: no idea what winaim expects here =) */
  }
  
  snprintf(path, strlen(sess->oft.listingdir)+strlen(fh->name)+2, "%s/%s", sess->oft.listingdir, fh->name);


  if( (file = fopen(path, "r")) == NULL) {
    printf("faimtest: getfile_send fopen failed for %s. damn.\n", path);
    return 0;
  }

  if (aim_getfile_send(oftconn, file, fh) == -1) {
    printf("faimtest: getfile_send failed. damn.\n");
  } else {
    printf("faimtest: looks like getfile went clean\n");
  }

  free(fh);  
  return 1;
}

int faimtest_getfile_complete(struct aim_session_t *sess, struct command_rx_struct *command, ...) 
{
  va_list ap;
  struct aim_conn_t *conn;
  struct aim_fileheader_t *fh;

  va_start(ap, command);
  conn = va_arg(ap, struct aim_conn_t *);
  fh = va_arg(ap, struct aim_fileheader_t *);
  va_end(ap);

  printf("faimtest: completed file transfer for %s.\n", fh->name);

  /*  aim_conn_kill(sess, &conn); */ /* we'll let winaim close the conn */
  return 1;
}

int faimtest_getfile_disconnect(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  struct aim_conn_t *conn;
  char *sn;

  va_start(ap, command);
  conn = va_arg(ap, struct aim_conn_t *);
  sn = va_arg(ap, char *);
  va_end(ap);

  aim_conn_kill(sess, &conn);

  printf("faimtest: getfile: disconnected from %s\n", sn);
  return 1;
}
#endif

int faimtest_parse_ratechange(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  static char *codes[5] = {"invalid",
			   "change",
			   "warning",
			   "limit",
			   "limit cleared"};
  va_list ap;
  int code;
  unsigned long parmid, windowsize, clear, alert, limit, disconnect;
  unsigned long currentavg, maxavg;

  va_start(ap, command); 

  /* See code explanations below */
  code = va_arg(ap, int);

  /*
   * Known parameter ID's...
   *   0x0001  Warnings
   *   0x0003  BOS (normal ICBMs, userinfo requests, etc)
   *   0x0005  Chat messages
   */
  parmid = va_arg(ap, unsigned long);

  /*
   * Not sure what this is exactly.  I think its the temporal 
   * relation factor (ie, how to make the rest of the numbers
   * make sense in the real world). 
   */
  windowsize = va_arg(ap, unsigned long);

  /* Explained below */
  clear = va_arg(ap, unsigned long);
  alert = va_arg(ap, unsigned long);
  limit = va_arg(ap, unsigned long);
  disconnect = va_arg(ap, unsigned long);
  currentavg = va_arg(ap, unsigned long);
  maxavg = va_arg(ap, unsigned long);

  va_end(ap);


  printf("faimtest: rate %s (paramid 0x%04lx): curavg = %ld, maxavg = %ld, alert at %ld, clear warning at %ld, limit at %ld, disconnect at %ld (window size = %ld)\n",
	 (code < 5)?codes[code]:"invalid",
	 parmid,
	 currentavg, maxavg,
	 alert, clear,
	 limit, disconnect,
	 windowsize);

  if (code == AIM_RATE_CODE_CHANGE) {
    /*
     * Not real sure when these get sent.
     */
    if (currentavg >= clear)
      aim_conn_setlatency(command->conn, 0);

  } else if (code == AIM_RATE_CODE_WARNING) {
    /*
     * We start getting WARNINGs the first time we go below the 'alert'
     * limit (currentavg < alert) and they stop when either we pause
     * long enough for currentavg to go above 'clear', or until we
     * flood it bad enough to go below 'limit' (and start getting
     * LIMITs instead) or even further and go below 'disconnect' and 
     * get disconnected completely (and won't be able to login right
     * away either).
     */
    aim_conn_setlatency(command->conn, windowsize/4); /* XXX this is bogus! */ 

  } else if (code == AIM_RATE_CODE_LIMIT) {
    /*
     * When we hit LIMIT, messages will start getting dropped.
     */
    aim_conn_setlatency(command->conn, windowsize/2); /* XXX this is bogus! */ 

  } else if (code == AIM_RATE_CODE_CLEARLIMIT) {
    /*
     * The limit is cleared when curavg goes above 'clear'.
     */
    aim_conn_setlatency(command->conn, 0); 
  }

  return 1;
}

int faimtest_parse_evilnotify(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  va_list ap;
  int newevil;
  struct aim_userinfo_s *userinfo;

  va_start(ap, command);
  newevil = va_arg(ap, int);
  userinfo = va_arg(ap, struct aim_userinfo_s *);
  va_end(ap);

  /*
   * Evil Notifications that are lacking userinfo->sn are anon-warns
   * if they are an evil increases, but are not warnings at all if its
   * a decrease (its the natural backoff happening).
   *
   * newevil is passed as an int representing the new evil value times
   * ten.
   */
  printf("faimtest: evil level change: new value = %2.1f%% (caused by %s)\n", ((float)newevil)/10, (userinfo && strlen(userinfo->sn))?userinfo->sn:"anonymous");

  return 1;
}
