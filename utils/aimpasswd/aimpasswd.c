/* 
 * aimpasswd.c -- Change AIM password without logging in.
 * 
 * Defintly not done yet.
 *
 * TODO: Make this work.
 *
 *  -----------------------------------------------------------
 *
 *  I'm releasing this code and all it's associated linkage
 *  under the GNU General Public License.  For more information,
 *  please refer to http://www.fsf.org.  For any questions,
 *  please contact me at the address below.
 *
 *  (c) 1998/99 Adam Fritzler, PST, afritz@iname.com
 *
 *  -----------------------------------------------------------
 *
 */

/*
  Current status:


 */

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

int bleu(struct command_rx_struct *blah, ...)
{
  return -1;
}

int main(void)
{
  /*
    specify your custom command handlers here.  I recommend
    at least overriding the login_phase3d_f handler so that
    you can have your own buddy list and profile.  The
    rest are probably only useful to override for UI
    reasons.
   */
  rxcallback_t faimtest_callbacks[] = {
    bleu, /* incoming IM */
    bleu, /* oncoming buddy */
    bleu, /* offgoing buddy */
    NULL, /* last IM was missed 1 */
    NULL, /* last IM was missed 2 */
    NULL, /* login phase 4 packet C command 1 -- depricated */
    NULL, /* login phase 4 packet C command 2 -- depricated */
    NULL, /* login phase 2, first resp -- depricated */
    faimtest_serverready, /* server ready -- **HANDLING REQUIRED** */
    NULL, /* login phase 3 packet B -- depricated */
    NULL, /* login phase 3D packet A -- depricated */
    NULL, /* login phase 3D packet B -- depricated */
    NULL, /* login phase 3D packet C -- depricated */
    NULL, /* login phase 3D packet D -- depricated */
    NULL, /* login phase 3D packet E -- depricated */
    faimtest_handleredirect, /* redirect -- **HANDLING REQUIRED** */
    NULL, /* last command bad */
    NULL, /* missed some messages */
    NULL, /* completely unknown command */
    bleu, /* user info response */
    NULL, /* user search by address response */
    NULL, /* user serach by name response */
    NULL, /* user search fail */
    faimtest_auth_error,
    faimtest_auth_success,
    faimtest_authsvrready,
    NULL,
    faimtest_pwdchngdone,
    faimtest_serverready,
    0x00 /* terminating NULL -- REQUIRED */
  };

  aim_connrst(); /* reset connection array -- there's a better place for this*/
  /* register our callback array (optional) */
  aim_register_callbacks(faimtest_callbacks);


  aim_login("NAME", "PASS");
  while (aim_select(NULL) > (struct aim_conn_t *)0)
    {
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

  /* Get out */
  exit(0);
}

int faimtest_serverready(struct command_rx_struct *command, ...)
{
  switch (command->conn->type)
    {
    case AIM_CONN_TYPE_BOS:
      printf("requesting AUTH service\n");
      aim_bos_reqservice(command->conn, 0x0007);
      break;
    default:
      fprintf(stderr, "faimtest: unknown connection type on Server Ready\n");
    }
  return 0;
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

  va_start(ap, command);
  serviceid = va_arg(ap, int);
  ip = va_arg(ap, char *);
  cookie = va_arg(ap, char *);
  va_end(ap);

  switch(serviceid)
    {
    case 0x0007: /* Authorizer */
      {
	struct aim_conn_t *tstconn;
	/* Open a connection to the Auth */
	tstconn = aim_newconn(AIM_CONN_TYPE_AUTH, ip);
	/* Send the cookie to the Auth */
	aim_auth_sendcookie(tstconn, cookie);
      }  
      break;
    default:
      printf("uh oh... got redirect for unknown service 0x%04x!!\n", serviceid);
      /* dunno */
    }

  return 0;
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
  aim_auth_sendcookie(bosconn, logininfo->cookie);

  return 0;
}

int faimtest_authsvrready(struct command_rx_struct *command, ...)
{
  /* should just be able to tell it we're ready too... */
  aim_auth_clientready(command->conn);

  /*
   * This is where you'd really begin changing your password.
   *   However, this callback may get called for reasons other
   *   than you wanting to change your password.  You should 
   *   probably check that before actually doing it.
   */
  //aim_auth_changepasswd(command->conn, "", "");

  return 0;
}

int faimtest_pwdchngdone(struct command_rx_struct *command, ...)
{
  printf("PASSWORD CHANGE SUCCESSFUL!!!\n");
  return 0;
}




