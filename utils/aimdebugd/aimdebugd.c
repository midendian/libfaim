
#include <faim/aim.h>
#include <signal.h>
#include <sys/wait.h>
#include "network.h"

#define RUNNAME "aimdebugd"
#define RUNPREFIX RUNNAME ": "

typedef struct {
  struct aim_session_t sess;
} client_t;

int stayalive = 1;

int clientready = 0;
int scriptfd = -1;

void sigchld(int signum)
{
  pid_t pid;
  int status;

  pid = wait(&status);
  
  return;
}

int cb_login(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  return 1;
}

int incomingim(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  int channel;
  va_list ap;

  va_start(ap, command);
  channel = va_arg(ap, int);

  if (channel == 1) {
    char *sn = NULL;
    char *msg = NULL;
    u_int icbmflags = 0;
    u_short flag1, flag2;
    
    sn = va_arg(ap, char *);
    msg = va_arg(ap, char *);
    icbmflags = va_arg(ap, u_int);
    flag1 = va_arg(ap, u_short);
    flag2 = va_arg(ap, u_short);
    va_end(ap);
    
    printf("aimdebugd: client %d: %s/0x%04x/0x%02x/0x%02x/%s\n",
	   getpid(),
	   sn,
	   icbmflags, flag1,
	   flag2, msg);

  } else
    printf("aimdebugd: client %d: unsupported ICBM channel %d\n", getpid(), channel);

  return 1;
}

int debugconn_connect(struct aim_session_t *sess, struct command_rx_struct *command, ...)
{
  struct aim_conn_t *scriptconn;

  clientready = 1;

  if (!(scriptconn = aim_newconn(sess, 0, NULL))) {
    printf(RUNPREFIX "unable to allocate script structures\n");
    aim_logoff(sess);
    return 1;
  }
  scriptconn->fd = scriptfd;

  return 1;
}

int parsescriptline(struct aim_session_t *sess, struct aim_conn_t **conn); /* file.c */

int handlechild(int fd, char *scriptname)
{     
  int alive;
  int selstat;
  client_t client;
  struct aim_conn_t *inconn, *waitingconn;

  aim_session_init(&client.sess, 0);
  //client.sess.tx_enqueue = aim_tx_enqueue__immediate;

  if (!(inconn = aim_newconn(&client.sess, AIM_CONN_TYPE_BOS, NULL))) {
    printf(RUNPREFIX "unable to allocate client structures\n");
    exit(-1);
  }
  inconn->fd = fd;

  if (scriptname) {
    if (scriptname[0] == '-') 
      scriptfd = STDIN_FILENO; 
    else if ((scriptfd = open(scriptname, O_RDONLY)) < 0) {
      perror(RUNPREFIX "open");
      return -1;
    }
  }

  aim_conn_addhandler(&client.sess, inconn, 0x0000, 0x0001, cb_login, 0);
  aim_conn_addhandler(&client.sess, inconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_DEBUGCONN_CONNECT, debugconn_connect, 0);
  aim_conn_addhandler(&client.sess, inconn, 0x0004, 0x0006, incomingim, 0);

  aim_debugconn_sendconnect(&client.sess, inconn);

  alive = 1;
  while (alive) {
    waitingconn = aim_select(&client.sess, NULL, &selstat);
    
    switch(selstat) {
    case -1: /* error */
      alive = 0; /* fall through and hit aim_logoff() */
      break;
    case 0: /* nothing pending */
      break;
    case 1: /* outgoing data pending */
      aim_tx_flushqueue(&client.sess);
      break;
    case 2: /* incoming data pending */
      if (waitingconn->fd == scriptfd) {
	if (clientready) {
	  if (parsescriptline(&client.sess, &waitingconn) < 0) {
	    alive = 0;
	  }
	}
      } else {
	if (aim_get_command(&client.sess, waitingconn) < 0) {
	  printf(RUNPREFIX "connection error\n");
	  alive = 0; /* fall through to aim_logoff() */
	} else {
	  aim_rxdispatch(&client.sess);
	}
      }
      break;
    default: /* invalid */
      break;
    }
  }

  printf(RUNPREFIX "client disconnected\n");

  close(fd);
  aim_logoff(&client.sess);

  return fd;
}

int main(int argc, char **argv)
{
  int listener = -1;
  int client = -1;
  int n;
  char *scriptname = NULL;
  int nofork = 0;
  int runonce = 0, runsleft = 1;
  
  while ((n = getopt(argc, argv, "c:noh")) != EOF) {
    switch(n) {
    case 'c':
      scriptname = optarg;
      break;
    case 'n': /* don't fork */
      nofork = 1;
      break;
    case 'o': /* run once only */
      runonce = 1;
      break;
    usage:
    case 'h':
      printf("aimdebugd v0.10 -- Adam Fritzler (mid@auk.cx)\n");
      printf("Usage:\n");
      printf("\taimdebugd [-c file] [-n] [-o]\n");
      printf("\t\t-c file\tScript file or - for stdin\n");
      printf("\t\t-n\tDo not fork for each client, process serially\n");
      printf("\t\t-o\tRun once and exit (required if script from stdin)\n");
      printf("\n");
      exit(2);
    }
  }

  if (scriptname && (scriptname[0] == '-') && (!nofork || !runonce)) {
    printf(RUNPREFIX "stdin script is not valid without -n and -o\n");
    return -1;
  }

  printf(RUNPREFIX "starting\n");

  signal(SIGCHLD, sigchld);
  
  if ((listener = establish(5190)) < 0) {
    perror(RUNPREFIX "establish");
    exit(-1);
  }

  while (stayalive) {
    if (runonce && !runsleft)
      break;
    if ((client = get_connection(listener)) < 0) {
      perror(RUNPREFIX "get_connection");
      stayalive = 0;
    } else {
      runsleft--;
      if (nofork)
	handlechild(client, scriptname);
      else {
	switch(fork()) {
	case -1:
	  perror(RUNPREFIX "fork");
	  break;
	case 0:
	  return handlechild(client, scriptname);
	  break;
	default:
	  close(client); /* let the child have it */
	  break;
	}
      }
    }
  }
  
  printf(RUNPREFIX "stopping\n");
  return 0;
}
