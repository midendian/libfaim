#ifndef __FAIMTEST_H__
#define __FAIMTEST_H__

#include <aim.h> 

extern int keepgoing;
extern aim_session_t aimsess;

/* This is kept in the aim_session_t and accessible by handlers. */
struct faimtest_priv {
	char *aimbinarypath;
	char *screenname;
	char *password;
	char *server;
	char *proxy;
	char *proxyusername;
	char *proxypass;
	char *ohcaptainmycaptain;
	int connected;

	FILE *listingfile;
	char *listingpath;

	fu8_t *buddyicon;
	int buddyiconlen;
	time_t buddyiconstamp;
	fu16_t buddyiconsum;
};

/* login.c */
int login(aim_session_t *sess, const char *sn, const char *passwd);
int logout(aim_session_t *sess);

/* commands.c */
void cmd_init(void);
void cmd_gotkey(void);
void cmd_uninit(void);

/* faimtest.c */
int faimtest_conncomplete(aim_session_t *sess, aim_frame_t *fr, ...);
int faimtest_flapversion(aim_session_t *sess, aim_frame_t *fr, ...);
int faimtest_init(void);
char *dprintf_ctime(void);
void addcb_bos(aim_session_t *sess, aim_conn_t *bosconn);

/* ft.c */
void getfile_start(aim_session_t *sess, aim_conn_t *conn, const char *sn);
void getfile_requested(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args);
void directim_start(aim_session_t *sess, aim_conn_t *conn, const char *sn);
void directim_requested(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args);

/* chat.c */
void chatnav_redirect(aim_session_t *sess, const char *ip, const fu8_t *cookie);
void chat_redirect(aim_session_t *sess, const char *ip, const fu8_t *cookie, const char *roomname, fu16_t exchange);

#define DPRINTF_OUTSTREAM stdout
#define dprintf(x) { \
  fprintf(DPRINTF_OUTSTREAM, "%s  %s: " x, dprintf_ctime(), "faimtest"); \
  fflush(DPRINTF_OUTSTREAM); \
}
#define dvprintf(x, y...) { \
  fprintf(DPRINTF_OUTSTREAM, "%s  %s: " x, dprintf_ctime(), "faimtest", y); \
  fflush(DPRINTF_OUTSTREAM); \
}
#define dinlineprintf(x) { \
  fprintf(DPRINTF_OUTSTREAM, x); \
  fflush(DPRINTF_OUTSTREAM); \
}
#define dvinlineprintf(x, y...) { \
  fprintf(DPRINTF_OUTSTREAM, x, y); \
  fflush(DPRINTF_OUTSTREAM); \
}
#define dperror(x) dvprintf("%s: %s\n", x, strerror(errno));


#endif /* __FAIMTEST_H__ */
