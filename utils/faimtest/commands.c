
#include "faimtest.h"
#include <readline/readline.h>
#include <readline/history.h>

static int cmd_help(char *arg);
static int cmd_quit(char *arg);
static int cmd_login(char *arg);
static int cmd_logout(char *arg);
static int cmd_connlist(char *arg);
static int cmd_goodday(char *arg);
static int cmd_warn(char *arg);
static int cmd_anonwarn(char *arg);
static int cmd_sendmsg(char *arg);

struct {
  char *name;
  Function *func;
  char *doc;
} cmdlist[] = {
  { "help", cmd_help, "Help"},
  { "quit", cmd_quit, "Quit"},
  { "login", cmd_login, "Log into AIM"},
  { "logout", cmd_logout, "Log out of AIM"},
  { "connlist", cmd_connlist, "List open connections"},
  { "goodday", cmd_goodday, "Say goodday to arg"},
  { "warn", cmd_warn, "Warn arg"},
  { "anonwarn", cmd_anonwarn, "Anonymously warn arg"},
  { "sendmsg", cmd_sendmsg, "Send arg[0] bytes to arg[1]"},
  { (char *)NULL, (Function *)NULL, (char *)NULL }
};

static char *stripwhite (char *string)
{
  char *s, *t;

  for (s = string; whitespace(*s); s++)
    ;

  if (*s == 0)
    return (s);

  t = s + strlen (s) - 1;
  while (t > s && whitespace (*t))
    t--;
  *++t = '\0';

  return s;
}

static char *cmdgenerator(char *text, int state)
{
  static int list_index, len;
  char *name;

  if (!state) {
    list_index = 0;
    len = strlen (text);
  }

  while ((name = cmdlist[list_index].name)) {
    list_index++;
    if (strncmp (name, text, len) == 0)
      return (strdup(name));
  }

  /* If no names matched, then return NULL. */
  return (char *)NULL;
}

static char **cmdcomplete(char *text, int start, int end)
{
  char **matches;

  matches = (char **)NULL;

  /* 
   * If this word is at the start of the line, then it is a command
   * to complete.  Otherwise it is the name of a file in the current
   * directory. 
   */
  if (start == 0)
    matches = completion_matches(text, cmdgenerator);

  return matches;
}

static Function *cmdfind(char *name)
{
  int i;

  for (i = 0; cmdlist[i].name; i++)
    if (strcmp (name, cmdlist[i].name) == 0)
      return cmdlist[i].func;

  return NULL;
}

static int cmdexec(char *line)
{
  int i;
  Function *cmd;
  char *word;

  /* Isolate the command word. */
  i = 0;
  while (line[i] && whitespace (line[i]))
    i++;
  word = line + i;

  while (line[i] && !whitespace (line[i]))
    i++;

  if (line[i])
    line[i++] = '\0';

  if (!(cmd = cmdfind(word))) {
    fprintf(stderr, "%s: invalid command\n", word);
    return -1;
  }
  /* Get argument to command, if any. */
  while (whitespace (line[i]))
    i++;

  word = line + i;

  /* Call the function. */
  return cmd(word);
}

static void fullline(void) 
{
  char *stripped;

  stripped = stripwhite(rl_line_buffer);

  if (*stripped) {
    add_history(stripped);
    cmdexec(stripped);
  }

  return;
}

void cmd_init(void)
{

  rl_attempted_completion_function = cmdcomplete;

  printf("Welcome to faimtest.\n");

  rl_callback_handler_install("faimtest> ", fullline);

  return;
}

void cmd_gotkey(void)
{

  rl_callback_read_char();

  return;
}

static int cmd_help(char *arg)
{
  int i;

  for (i = 0; cmdlist[i].name; i++)
    printf("%16s\t\t%s\n", cmdlist[i].name, cmdlist[i].doc);

  return 0;
}

static int cmd_quit(char *arg)
{
  keepgoing = 0;

  return 0;
}

static int cmd_login(char *arg)
{
  char *sn = NULL, *passwd = NULL;

  if (arg) {
    sn = arg;
    if ((passwd = index(sn, ' '))) {
      *(passwd) = '\0';
      passwd++;
    }
  }

  if (login(sn, passwd) != 0)
    printf("login failed\n");

  return 0;
}

static int cmd_logout(char *arg)
{
  logout();

  return 0;
}

static int cmd_connlist(char *arg)
{
  struct aim_conn_t *cur;

  printf("Open connections:\n");
  for (cur = aimsess.connlist; cur; cur = cur->next) {
    printf(" fd=%d  type=0x%02x\n", cur->fd, cur->type);
  }

  return 0;
}

static int cmd_goodday(char *arg)
{
  if (arg && strlen(arg) && (strlen(arg) < MAXSNLEN))
    aim_send_im(&aimsess, aim_getconn_type(&aimsess, AIM_CONN_TYPE_BOS), arg, AIM_IMFLAGS_ACK, "Good day to you too.",  strlen("Good day to you too."));
  else
    printf("no one to say hello to!\n");

  return 0;
}

static int cmd_warn(char *arg)
{
  if (arg && strlen(arg) && (strlen(arg) < MAXSNLEN))
    aim_send_warning(&aimsess, aim_getconn_type(&aimsess, AIM_CONN_TYPE_BOS), arg, 0);
  else
    printf("no one to warn!\n");

  return 0;
}

static int cmd_anonwarn(char *arg)
{
  if (arg && strlen(arg) && (strlen(arg) < MAXSNLEN))
    aim_send_warning(&aimsess, aim_getconn_type(&aimsess, AIM_CONN_TYPE_BOS), arg, AIM_WARN_ANON);
  else
    printf("no one to anonwarn!\n");

  return 0;
}

static int cmd_sendmsg(char *arg)
{
  int len = 0, z;
  char sn[MAXSNLEN+1], *newbuf = NULL;

  if ((sscanf(arg, "%d %32s", &len, sn) != 2) || 
      (len >= 10000) || (strlen(sn) > MAXSNLEN)) {
    printf("invalid args\n");
    return 0;
  }

  printf("sending %d bytes to %s\n", len, sn);

  if (!(newbuf = malloc(len+1)))
    return 0;

  for (z = 0; z < len; z++)
    newbuf[z] = (z % 10)+0x30;
  newbuf[len] = '\0';

  aim_send_im(&aimsess, aim_getconn_type(&aimsess, AIM_CONN_TYPE_BOS), sn, AIM_IMFLAGS_ACK, newbuf, strlen(newbuf));

  free(newbuf);

  return 0;
}

void cmd_uninit(void)
{

  rl_callback_handler_remove();

  return;
}
