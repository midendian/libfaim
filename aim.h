#ifndef __AIM_H__
#define __AIM_H__

#include <faimconfig.h>

#include <stdio.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <stdlib.h>
#include <stdarg.h>
#include <errno.h>

#ifdef _WIN32
#include <windows.h>
#include <time.h>
#include <io.h>
#else
#include <netdb.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/time.h>
#include <unistd.h>
#endif

/* Portability stuff (DMP) */

#ifdef _WIN32
#define sleep Sleep
#define strlen(x) (int)strlen(x)  /* win32 has a unsigned size_t */
#endif

#if defined(_WIN32) || (defined(mach) && defined(__APPLE__)) 
#define gethostbyname2(x,y) gethostbyname(x) /* revert to IPv4 */
#endif 

/*
 * Login Error codes
 */
#define AIM_CONNECT_ERROR	-0x1
#define AIM_SIGNON_TOO_SOON	-0x4
#define AIM_SERVICE_FULL	-0x6f

/* Current Maximum Length for Screen Names (not including NULL) */
#define MAXSNLEN 16

struct login_phase1_struct {
  char *screen_name;
  char *BOSIP;
  char *cookie;
  char *email;
  u_short regstatus;
};

extern struct login_phase1_struct aim_logininfo;

struct client_info_s {
  char clientstring[100]; /* arbitrary number */
  int major;
  int minor;
  int build;
  char country[3];
  char lang[3];
};

struct connection_info_struct {
  u_int local_seq_num_origin; /* our first seq num */
  int local_command_count;

  u_int remote_seq_num_origin; /* oscar's first seqnum */
  int remote_command_count; /* command_count + seq_num_origin = cur_seq_num */

  char *sn; /* our screen name */

  int fd;                   /* socket descriptor */
};

#ifndef TRUE
#define TRUE 1
#define FALSE 0
#endif

#define AIM_CONN_MAX 5 
/* these could be arbitrary, but its easier to use the actual AIM values */
#define AIM_CONN_TYPE_AUTH 0x0007
#define AIM_CONN_TYPE_ADS 0x0005
#define AIM_CONN_TYPE_BOS 2
#define AIM_CONN_TYPE_CHAT 0x000e
#define AIM_CONN_TYPE_CHATNAV 0x000d

#define AIM_CONN_STATUS_READY 0x0001
#define AIM_CONN_STATUS_INTERNALERR 0x0002
#define AIM_CONN_STATUS_RESOLVERR 0x80
#define AIM_CONN_STATUS_CONNERR 0x40


struct aim_conn_t {
  int fd;
  int type;
  int seqnum;
  int status;
  time_t lastactivity; /* time of last transmit */
  int forcedlatency; 
  struct aim_rxcblist_t *handlerlist;
};
struct aim_conn_t aim_conns[AIM_CONN_MAX];


/* struct for incoming commands */
struct command_rx_struct {
                            /* byte 1 assumed to always be 0x2a */
  char type;                /* type code (byte 2) */
  u_int seqnum;             /* sequence number (bytes 3 and 4) */
  u_int commandlen;         /* total packet len - 6 (bytes 5 and 6) */
  u_char *data;             /* packet data (from 7 byte on) */
  u_int lock;               /* 0 = open, !0 = locked */
  u_int handled;            /* 0 = new, !0 = been handled */
  struct aim_conn_t *conn;  /* the connection it came in on... */
  struct command_rx_struct *next; /* ptr to next struct in list */
};

/* struct for outgoing commands */
struct command_tx_struct {
                            /* byte 1 assumed to be 0x2a */
  char type;                /* type/family code */
  u_int seqnum;             /* seqnum dynamically assigned on tx */
  u_int commandlen;         /* SNAC length */
  u_char *data;             /* packet data */
  u_int lock;               /* 0 = open, !0 = locked */
  u_int sent;               /* 0 = pending, !0 = has been sent */
  struct aim_conn_t *conn; 
  struct command_tx_struct *next; /* ptr to next struct in list */
};

/*
 * AIM User Info, Standard Form.
 */
struct aim_userinfo_s {
  char sn[MAXSNLEN+1];
  u_short warnlevel;
  u_short idletime;
  u_short class;
  u_long membersince;
  u_long onlinesince;
  u_long sessionlen;  
};

/* TLV-related tidbits */
struct aim_tlv_t {
  u_short type;
  u_short length;
  u_char *value;
};

struct aim_tlvlist_t {
  struct aim_tlv_t *tlv;
  struct aim_tlvlist_t *next;
};
struct aim_tlvlist_t *aim_readtlvchain(u_char *buf, int maxlen);
void aim_freetlvchain(struct aim_tlvlist_t **list);
struct aim_tlv_t *aim_grabtlv(u_char *src);
struct aim_tlv_t *aim_grabtlvstr(u_char *src);
int aim_puttlv (u_char *dest, struct aim_tlv_t *newtlv);
struct aim_tlv_t *aim_createtlv(void);
int aim_freetlv(struct aim_tlv_t **oldtlv);
int aim_puttlv_16(u_char *, u_short, u_short);

/* some prototypes... */

/*   implicitly or explicitly called */
int aim_get_command(void);
int aim_rxdispatch(void);
int aim_logoff(void);

typedef int (*rxcallback_t)(struct command_rx_struct *, ...);
int aim_register_callbacks(rxcallback_t *);

u_long aim_genericreq_n(struct aim_conn_t *conn, u_short family, u_short subtype);
u_long aim_genericreq_l(struct aim_conn_t *conn, u_short family, u_short subtype, u_long *);
u_long aim_genericreq_s(struct aim_conn_t *conn, u_short family, u_short subtype, u_short *);

/* aim_login.c */
int aim_send_login (struct aim_conn_t *, char *, char *, struct client_info_s *);
int aim_encode_password(const char *, char *);


struct command_rx_struct *aim_purge_rxqueue(struct command_rx_struct *queue);


int aim_parse_unknown(struct command_rx_struct *command, ...);
int aim_parse_missed_im(struct command_rx_struct *, ...);
int aim_parse_last_bad(struct command_rx_struct *, ...);

int aim_tx_enqueue(struct command_tx_struct *);
u_int aim_get_next_txseqnum(struct aim_conn_t *);
int aim_tx_flushqueue(void);
int aim_tx_printqueue(void);
int aim_tx_purgequeue(void);

/* queue (linked list) pointers */
extern struct command_tx_struct *aim_queue_outgoing; /* incoming commands */
extern struct command_rx_struct *aim_queue_incoming; /* outgoing commands */

/* The default callback handler array */
extern rxcallback_t aim_callbacks[];

struct aim_rxcblist_t {
  u_short family;
  u_short type;
  rxcallback_t handler;
  u_short flags;
  struct aim_rxcblist_t *next;
};

int aim_conn_setlatency(struct aim_conn_t *conn, int newval);

int aim_conn_addhandler(struct aim_conn_t *conn, u_short family, u_short type, rxcallback_t newhandler, u_short flags);
rxcallback_t aim_callhandler(struct aim_conn_t *conn, u_short family, u_short type);
int aim_clearhandlers(struct aim_conn_t *conn);

extern struct aim_snac_t *aim_outstanding_snacs;
extern u_long aim_snac_nextid;

#define AIM_CB_FAM_ACK 0x0000
#define AIM_CB_FAM_GEN 0x0001
#define AIM_CB_FAM_LOC 0x0002
#define AIM_CB_FAM_BUD 0x0003
#define AIM_CB_FAM_MSG 0x0004
#define AIM_CB_FAM_ADS 0x0005
#define AIM_CB_FAM_INV 0x0006
#define AIM_CB_FAM_ADM 0x0007
#define AIM_CB_FAM_POP 0x0008
#define AIM_CB_FAM_BOS 0x0009
#define AIM_CB_FAM_LOK 0x000a
#define AIM_CB_FAM_STS 0x000b
#define AIM_CB_FAM_TRN 0x000c
#define AIM_CB_FAM_CTN 0x000d /* ChatNav */
#define AIM_CB_FAM_CHT 0x000e /* Chat */
#define AIM_CB_FAM_SPECIAL 0xffff /* Internal libfaim use */

#define AIM_CB_ACK_ACK 0x0001

#define AIM_CB_GEN_ERROR 0x0001
#define AIM_CB_GEN_CLIENTREADY 0x0002
#define AIM_CB_GEN_SERVERREADY 0x0003
#define AIM_CB_GEN_SERVICEREQ 0x0004
#define AIM_CB_GEN_REDIRECT 0x0005
#define AIM_CB_GEN_RATEINFOREQ 0x0006
#define AIM_CB_GEN_RATEINFO 0x0007
#define AIM_CB_GEN_RATEINFOACK 0x0008
#define AIM_CB_GEN_RATECHANGE 0x000a
#define AIM_CB_GEN_SERVERPAUSE 0x000b
#define AIM_CB_GEN_SERVERRESUME 0x000d
#define AIM_CB_GEN_REQSELFINFO 0x000e
#define AIM_CB_GEN_SELFINFO 0x000f
#define AIM_CB_GEN_EVIL 0x0010
#define AIM_CB_GEN_SETIDLE 0x0011
#define AIM_CB_GEN_MIGRATIONREQ 0x0012
#define AIM_CB_GEN_MOTD 0x0013
#define AIM_CB_GEN_SETPRIVFLAGS 0x0014
#define AIM_CB_GEN_WELLKNOWNURL 0x0015
#define AIM_CB_GEN_NOP 0x0016
#define AIM_CB_GEN_DEFAULT 0xffff

#define AIM_CB_LOC_ERROR 0x0001
#define AIM_CB_LOC_REQRIGHTS 0x0002
#define AIM_CB_LOC_RIGHTSINFO 0x0003
#define AIM_CB_LOC_SETUSERINFO 0x0004
#define AIM_CB_LOC_REQUSERINFO 0x0005
#define AIM_CB_LOC_USERINFO 0x0006
#define AIM_CB_LOC_WATCHERSUBREQ 0x0007
#define AIM_CB_LOC_WATCHERNOT 0x0008
#define AIM_CB_LOC_DEFAULT 0xffff

#define AIM_CB_BUD_ERROR 0x0001
#define AIM_CB_BUD_REQRIGHTS 0x0002
#define AIM_CB_BUD_RIGHTSINFO 0x0003
#define AIM_CB_BUD_ADDBUDDY 0x0004
#define AIM_CB_BUD_REMBUDDY 0x0005
#define AIM_CB_BUD_REJECT 0x000a
#define AIM_CB_BUD_ONCOMING 0x000b
#define AIM_CB_BUD_OFFGOING 0x000c
#define AIM_CB_BUD_DEFAULT 0xffff

#define AIM_CB_MSG_ERROR 0x0001
#define AIM_CB_MSG_PARAMINFO 0x0005
#define AIM_CB_MSG_INCOMING 0x0007
#define AIM_CB_MSG_EVIL 0x0009
#define AIM_CB_MSG_MISSEDCALL 0x000a
#define AIM_CB_MSG_CLIENTERROR 0x000b
#define AIM_CB_MSG_ACK 0x000c
#define AIM_CB_MSG_DEFAULT 0xffff

#define AIM_CB_ADS_ERROR 0x0001
#define AIM_CB_ADS_DEFAULT 0xffff

#define AIM_CB_INV_ERROR 0x0001
#define AIM_CB_INV_DEFAULT 0xffff

#define AIM_CB_ADM_ERROR 0x0001
#define AIM_CB_ADM_INFOCHANGE_REPLY 0x0005
#define AIM_CB_ADM_DEFAULT 0xffff

#define AIM_CB_POP_ERROR 0x0001
#define AIM_CB_POP_DEFAULT 0xffff

#define AIM_CB_BOS_ERROR 0x0001
#define AIM_CB_BOS_DEFAULT 0xffff

#define AIM_CB_LOK_ERROR 0x0001
#define AIM_CB_LOK_DEFAULT 0xffff

#define AIM_CB_STS_ERROR 0x0001
#define AIM_CB_STS_SETREPORTINTERVAL 0x0002
#define AIM_CB_STS_REPORTACK 0x0004
#define AIM_CB_STS_DEFAULT 0xffff

#define AIM_CB_TRN_ERROR 0x0001
#define AIM_CB_TRN_DEFAULT 0xffff

#define AIM_CB_CTN_ERROR 0x0001
#define AIM_CB_CTN_DEFAULT 0xffff

#define AIM_CB_CHT_ERROR 0x0001
#define AIM_CB_CHT_DEFAULT 0xffff

#define AIM_CB_SPECIAL_AUTHSUCCESS 0x0001
#define AIM_CB_SPECIAL_AUTHOTHER 0x0002
#define AIM_CB_SPECIAL_UNKNOWN 0xffff
#define AIM_CB_SPECIAL_DEFAULT AIM_CB_SPECIAL_UNKNOWN

#if 0
#define AIM_CB_INCOMING_IM 0
#define AIM_CB_ONCOMING_BUDDY 1
#define AIM_CB_OFFGOING_BUDDY 2
#define AIM_CB_MISSED_IM 3
#define AIM_CB_MISSED_CALL 4
#define AIM_CB_LOGIN_P4_C1 5
#define AIM_CB_LOGIN_P4_C2 6
#define AIM_CB_LOGIN_P2_1 7
#define AIM_CB_LOGIN_P2_2 8
#define AIM_CB_LOGIN_P3_B 9
#define AIM_CB_LOGIN_P3D_A 10
#define AIM_CB_LOGIN_P3D_B 11
#define AIM_CB_LOGIN_P3D_C 12
#define AIM_CB_LOGIN_P3D_D 13
#define AIM_CB_LOGIN_P3D_E 14
#define AIM_CB_LOGIN_P3D_F 15
#define AIM_CB_RATECHANGE 16
#define AIM_CB_USERERROR 17
#define AIM_CB_UNKNOWN 18
#define AIM_CB_USERINFO 19
#define AIM_CB_SEARCH_ADDRESS 20
#define AIM_CB_SEARCH_NAME 21
#define AIM_CB_SEARCH_FAIL 22

#define AIM_CB_AUTH_ERROR 23
#define AIM_CB_AUTH_SUCCESS 24
#define AIM_CB_AUTH_SVRREADY 25

#define AIM_CB_AUTH_OTHER 26
#define AIM_CB_AUTH_INFOCHNG_REPLY 27
#define AIM_CB_CHATNAV_SVRREADY 28
#endif

int Read(int, u_char *, int);

struct aim_snac_t {
  u_long id;
  u_short family;
  u_short type;
  u_short flags;
  void *data;
  time_t issuetime;
  struct aim_snac_t *next;
};
u_long aim_newsnac(struct aim_snac_t *newsnac);
struct aim_snac_t *aim_remsnac(u_long id);
int aim_cleansnacs(int maxage);
int aim_putsnac(u_char *, int, int, int, u_long);

void aim_connrst(void);
struct aim_conn_t *aim_conn_getnext(void);
void aim_conn_close(struct aim_conn_t *deadconn);
struct aim_conn_t *aim_getconn_type(int type);
struct aim_conn_t *aim_newconn(int type, char *dest);
int aim_conngetmaxfd(void);
struct aim_conn_t *aim_select(struct timeval *);
int aim_conn_isready(struct aim_conn_t *);
int aim_conn_setstatus(struct aim_conn_t *, int);

/* aim_misc.c */

#define AIM_VISIBILITYCHANGE_PERMITADD 0x05
#define AIM_VISIBILITYCHANGE_PERMITREMOVE 0x06
#define AIM_VISIBILITYCHANGE_DENYADD 0x07
#define AIM_VISIBILITYCHANGE_DENYREMOVE 0x08

u_long aim_bos_setidle(struct aim_conn_t *, u_long);
u_long aim_bos_changevisibility(struct aim_conn_t *, int, char *);
u_long aim_bos_setbuddylist(struct aim_conn_t *, char *);
u_long aim_bos_setprofile(struct aim_conn_t *, char *);
u_long aim_bos_setgroupperm(struct aim_conn_t *, u_long);
u_long aim_bos_clientready(struct aim_conn_t *);
u_long aim_bos_reqrate(struct aim_conn_t *);
u_long aim_bos_ackrateresp(struct aim_conn_t *);
u_long aim_bos_setprivacyflags(struct aim_conn_t *, u_long);
u_long aim_bos_reqpersonalinfo(struct aim_conn_t *);
u_long aim_bos_reqservice(struct aim_conn_t *, u_short);
u_long aim_bos_reqrights(struct aim_conn_t *);
u_long aim_bos_reqbuddyrights(struct aim_conn_t *);
u_long aim_bos_reqlocaterights(struct aim_conn_t *);
u_long aim_bos_reqicbmparaminfo(struct aim_conn_t *);

/* aim_rxhandlers.c */
int aim_register_callbacks(rxcallback_t *);
int aim_rxdispatch(void);
int aim_authparse(struct command_rx_struct *);
int aim_handleredirect_middle(struct command_rx_struct *, ...);
int aim_parse_unknown(struct command_rx_struct *, ...);
int aim_parse_missed_im(struct command_rx_struct *, ...);
int aim_parse_last_bad(struct command_rx_struct *, ...);
int aim_parse_generalerrs(struct command_rx_struct *command, ...);

/* aim_im.c */
#define AIM_IMFLAGS_AWAY 0x01 /* mark as an autoreply */
#define AIM_IMFLAGS_ACK 0x02 /* request a receipt notice */
u_long aim_send_im(struct aim_conn_t *, char *, int, char *);
int aim_parse_incoming_im_middle(struct command_rx_struct *);
u_long aim_seticbmparam(struct aim_conn_t *conn);

/* aim_info.c */
u_long aim_getinfo(struct aim_conn_t *, const char *);
int aim_extractuserinfo(u_char *, struct aim_userinfo_s *);
int aim_parse_userinfo_middle(struct command_rx_struct *);
int aim_parse_oncoming_middle(struct command_rx_struct *);

/* aim_auth.c */
int aim_auth_sendcookie(struct aim_conn_t *, char *);
u_long aim_auth_clientready(struct aim_conn_t *);
u_long aim_auth_changepasswd(struct aim_conn_t *, char *, char *);

/* aim_buddylist.c */
u_long aim_add_buddy(struct aim_conn_t *, char *);
u_long aim_remove_buddy(struct aim_conn_t *, char *);

/* aim_search.c */
u_long aim_usersearch_address(struct aim_conn_t *, char *);
/* u_long aim_usersearch_name(struct aim_conn_t *, char *); */

/* aim_util.c */
int aimutil_put8(u_char *, u_char);
u_char aimutil_get8(u_char *buf);
int aimutil_put16(u_char *, u_short);
u_short aimutil_get16(u_char *);
int aimutil_put32(u_char *, u_long);
u_long aimutil_get32(u_char *);
int aimutil_putstr(u_char *, const u_char *, int);
int aimutil_tokslen(char *toSearch, int index, char dl);
int aimutil_itemcnt(char *toSearch, char dl);
char *aimutil_itemidx(char *toSearch, int index, char dl);

struct aim_tlv_t *aim_gettlv(struct aim_tlvlist_t *list, u_short type, int nth);
char *aim_gettlv_str(struct aim_tlvlist_t *list, u_short type, int nth);

#endif /* __AIM_H__ */

