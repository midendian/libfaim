
#include <faim/aim.h>

static int readln(int fd, char *buf, int buflen)
{
  int i = 0;
  int ret = 0;

  while ((i < buflen) && ((ret = read(fd, buf, 1)) > 0)) {
    if (*buf == '\n') {
      *buf = '\0';
      return i;
    }
    buf++;
    i++;
  }

  if (ret == 0) {
    *buf = '\0';
    return i;
  }

  if (ret == -1)
    perror("read");

  return -1;
}

static inline char *killwhite(char *inbuf)
{
  char *buf;
  buf = inbuf;
  while(buf && ((*buf==' ') || (*buf=='\t'))) /* skip leading whitespace */
    buf++;
  return buf;
}

static inline char *nextwhite(char *inbuf)
{
  char *buf;
  buf = inbuf;
  while(buf) {
    if ((*buf != ' '))
      buf++;
    else
      return buf;
  }
  return buf;
}

int parsescriptline2(struct aim_session_t *sess, struct aim_conn_t **scriptconn, char *buf)
{
  if (!buf)
    return 0;

  switch (buf[0]) {
  case '#': /* comment */
    break;
  case 'b': /* break */
    aim_conn_kill(sess, scriptconn);
    break;
  case 'd': /* disconnect */
    aim_conn_kill(sess, scriptconn);
    aim_logoff(sess);
    return -1;
    break;
  case 'e': /* print to stderr */
    buf = nextwhite(buf)+1;
    fprintf(stderr, "%s\n", buf);
    break;
  case 'm': /* message */
    {  
      char *sn, *msg;
      buf = nextwhite(buf)+1;
      sn = aim_strsep(&buf, "/");
      msg = aim_strsep(&buf, "/");
      sendimtoclient(sess, aim_getconn_type(sess, AIM_CONN_TYPE_BOS), sn, 0, msg);
    }
    break;
  case 's': /* sleep */
    buf = nextwhite(buf)+1;
    printf("sleeping for %d seconds\n", atoi(buf));
    sleep(atoi(buf));
    break;
  default:
    printf("unknown script command %c\n", buf[0]);
    break;
  }
  return 0;
}

int parsescriptline(struct aim_session_t *sess, struct aim_conn_t **conn)
{
  char buf[256];
  if (readln((*conn)->fd, buf, 255) > 0) 
    return parsescriptline2(sess, conn, buf);
  else {
    aim_conn_kill(sess, conn);
  }
  return 0;
}
