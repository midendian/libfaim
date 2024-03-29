/* 
 *  faimtest.
 *
 *  The point of faimtest is twofold:
 *     - Test the functionality of libfaim.
 *     - Demonstrate the functionality of libfaim and how to use it.
 *
 *  It does the latter rather badly, and the first as best it can without a
 *  more realistic UI.  libfaim has a slightly different event model than
 *  many C programmers are used to, which is why I provide faimtest as
 *  documentation instead of attempting to explain how it works in English.
 *  If you're still in need of more guidance, see the source for the OSCAR
 *  "plugin" in gaim.  It does it nicely, and in a realistic situation. (Did
 *  I mention faimtest is a bit idealized?)
 *
 *  The faimtest code is very ugly.  Probably always will be.  
 *
 *  Note that faimtest does not do a lot of error checking, except perhaps
 *  on some libfaim funtions.  This is done for clarity, in hopes of
 *  making this crap ever so slighly more readable.
 *
 */

#include "faimtest.h"
#include <sys/stat.h>

char *dprintf_ctime(void)
{
	static char retbuf[64];
	struct tm *lt;
	struct timeval tv;
	struct timezone tz;

	gettimeofday(&tv, &tz);
	lt = localtime((time_t *)&tv.tv_sec);
	strftime(retbuf, 64, "%a %b %e %H:%M:%S %Z %Y", lt);

	return retbuf;
}

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
	"Not while on AOL",
};
static int msgerrreasonslen = 25;

aim_session_t aimsess;
int keepgoing = 1;

/* 
 * This is used to intercept debugging/diagnostic messages from libfaim.
 *
 * Note that you should have one of these even if you use a debuglevel of
 * zero, as libfaim will send serious errors to stderr by default.
 *
 */
static void faimtest_debugcb(aim_session_t *sess, int level, const char *format, va_list va)
{

	vfprintf(stderr, format, va);

	return;
}

int faimtest_flapversion(aim_session_t *sess, aim_frame_t *fr, ...)
{

	/* XXX fix libfaim to support this */
	dvprintf("using FLAP version 0x%08x\n", /* aimutil_get32(fr->data)*/ 0xffffffff);

#if 0
	/* 
	 * This is an alternate location for starting the login process.
	 */
	/* XXX should do more checking to make sure its really the right AUTH conn */
	if (fr->conn->type == AIM_CONN_TYPE_AUTH) {
		/* do NOT send a flapversion, request_login will send it if needed */
		aim_request_login(sess, fr->conn, priv->screenname);
		dprintf("faimtest: login request sent\n");
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
int faimtest_conncomplete(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_conn_t *conn;

	va_start(ap, fr);
	conn = va_arg(ap, aim_conn_t *);
	va_end(ap);

	if (conn)
		dvprintf("connection on %d completed\n", conn->fd);

	return 1;
}

#ifdef _WIN32
/*
 * This is really all thats needed to link against libfaim on win32.
 *
 * Note that this particular version of faimtest has never been tested
 * on win32, but I'm fairly sure it should work.
 */
static int initwsa(void)
{
	WORD wVersionRequested;
	WSADATA wsaData;

	wVersionRequested = MAKEWORD(2,2);
	return WSAStartup(wVersionRequested, &wsaData);
}
#endif /* _WIN32 */

/*
 * This is unrealistic.  Most clients will not be able to do this.
 */
int faimtest_init(void)
{
	aim_conn_t *stdinconn = NULL;

	if (!(stdinconn = aim_newconn(&aimsess, 0, NULL))) {
		dprintf("unable to create connection for stdin!\n");
		return -1;
	}

	stdinconn->fd = STDIN_FILENO;

	return 0;
}

int main(int argc, char **argv)
{
	aim_conn_t *waitingconn = NULL;
	int i;
	int selstat = 0;
	static int faimtest_mode = 0;
	struct timeval tv;
	time_t lastnop = 0;
	const char *buddyiconpath = NULL;
	struct faimtest_priv priv = {
		NULL, NULL, NULL, NULL,
		NULL, NULL, NULL, NULL,
		0,
		NULL, NULL,
		NULL, 0, 0, 0
	};

	priv.screenname = getenv("SCREENNAME");
	priv.password = getenv("PASSWORD");
	priv.server = getenv("AUTHSERVER");
	priv.proxy = getenv("SOCKSPROXY");
	priv.proxyusername = getenv("SOCKSNAME");
	priv.proxypass = getenv("SOCKSPASS");

	priv.listingpath = getenv("LISTINGPATH");

	while ((i = getopt(argc, argv, "u:p:a:U:P:A:l:c:hoOb:i:")) != EOF) {
		switch (i) {
		case 'u': priv.screenname = optarg; break;
		case 'p': priv.password = optarg; break;
		case 'a': priv.server = optarg; break;
		case 'U': priv.proxyusername = optarg; break;
		case 'P': priv.proxypass = optarg; break;
		case 'A': priv.proxy = optarg; break;
		case 'l': priv.listingpath = optarg; break;
		case 'c': priv.ohcaptainmycaptain = optarg; break;
		case 'o': faimtest_mode = 1; break; /* half old interface */
		case 'O': faimtest_mode = 2; break; /* full old interface */
		case 'b': priv.aimbinarypath = optarg; break;
		case 'i': buddyiconpath = optarg; break;
		case 'h':
		default:
			printf("faimtest\n");
			printf(" Options: \n");
			printf("    -u name       Screen name ($SCREENNAME)\n");
			printf("    -p passwd     Password ($PASSWORD)\n");
			printf("    -a host:port  Authorizer ($AUTHSERVER)\n");
			printf("    -U name       Proxy user name ($SOCKSPROXY)\n");
			printf("    -P passwd     Proxy password ($SOCKSNAME)\n");
			printf("    -A host:port  Proxy host ($SOCKSPASS)\n");
			printf("    -l path       Path to listing file ($LISTINGPATH)\n");
			printf("    -c name       Screen name of owner\n");
			printf("    -o            Login at startup, then prompt\n");
			printf("    -O            Login, never give prompt\n");
			printf("    -b path       Path to AIM 3.5.1670 binaries\n");
			printf("    -i file       Buddy Icon to send\n");
			exit(0);
		}
	}

#ifdef _WIN32
	if (initwsa() != 0) {
		dprintf("could not initialize windows sockets\n");
		return -1;
	}
#endif /* _WIN32 */

	/* Pass zero as flags if you want blocking connects */
	aim_session_init(&aimsess, AIM_SESS_FLAGS_NONBLOCKCONNECT, 1);
	aim_setdebuggingcb(&aimsess, faimtest_debugcb); /* still needed even if debuglevel = 0 ! */
	aimsess.aux_data = &priv;

	if (priv.listingpath) {
		char *listingname;
		
		listingname = (char *)calloc(1, strlen(priv.listingpath)+strlen("/listing.txt"));
		sprintf(listingname, "%s/listing.txt", priv.listingpath);
		
		if ((priv.listingfile = fopen(listingname, "r")) == NULL)
			dvprintf("Couldn't open %s... disabling that shit.\n", listingname);

		free(listingname);
	}

	if (buddyiconpath) {
		struct stat st;
		FILE *f;

		if ((stat(buddyiconpath, &st) != -1) && (st.st_size <= MAXICONLEN) && (f = fopen(buddyiconpath, "r"))) {

			priv.buddyiconlen = st.st_size;
			priv.buddyiconstamp = st.st_mtime;
			priv.buddyicon = malloc(priv.buddyiconlen);
			fread(priv.buddyicon, 1, st.st_size, f);

			priv.buddyiconsum = aim_iconsum(priv.buddyicon, priv.buddyiconlen);

			dvprintf("read %d bytes of %s for buddy icon (sum 0x%08x)\n", priv.buddyiconlen, buddyiconpath, priv.buddyiconsum);

			fclose(f);

		} else
			dvprintf("could not open buddy icon %s\n", buddyiconpath);

	}

	faimtest_init();

	if (faimtest_mode < 2)
		cmd_init();

	if (faimtest_mode >= 1) {
		if (login(&aimsess, priv.screenname, priv.password) == -1) {
			if (faimtest_mode < 2)
				cmd_uninit();
			exit(-1);
		}
	}

	while (keepgoing) {

		/* XXX uh. */
		tv.tv_sec = 5;
		tv.tv_usec = 0;

		waitingconn = aim_select(&aimsess, &tv, &selstat);

		if (priv.connected && ((time(NULL) - lastnop) > 30)) {
			lastnop = time(NULL);
			aim_flap_nop(&aimsess, aim_getconn_type(&aimsess, AIM_CONN_TYPE_BOS));
		}

		if (selstat == -1) { /* error */
			keepgoing = 0; /* fall through */
		} else if (selstat == 0) { /* no events pending */
			;
		} else if (selstat == 1) { /* outgoing data pending */
			aim_tx_flushqueue(&aimsess);
		} else if (selstat == 2) { /* incoming data pending */
			if ((faimtest_mode < 2) && (waitingconn->fd == STDIN_FILENO)) {
				cmd_gotkey();
			} else {
				if (waitingconn->type == AIM_CONN_TYPE_RENDEZVOUS_OUT) {
					if (aim_handlerendconnect(&aimsess, waitingconn) < 0) {
						dprintf("connection error (rend out)\n");
						aim_conn_kill(&aimsess, &waitingconn);
					}
				} else {
					if (aim_get_command(&aimsess, waitingconn) >= 0) {
						aim_rxdispatch(&aimsess);
					} else {
						dvprintf("connection error (type 0x%04x:0x%04x)\n", waitingconn->type, waitingconn->subtype);
						/* we should have callbacks for all these, else the library will do the conn_kill for us. */
						if (waitingconn->type == AIM_CONN_TYPE_RENDEZVOUS) {
							if (waitingconn->subtype == AIM_CONN_SUBTYPE_OFT_DIRECTIM)
								dvprintf("disconnected from %s\n", aim_directim_getsn(waitingconn));
							aim_conn_kill(&aimsess, &waitingconn);
						} else
							aim_conn_kill(&aimsess, &waitingconn);
						if (!aim_getconn_type(&aimsess, AIM_CONN_TYPE_BOS)) {
							dprintf("major connection error\n");
							if (faimtest_mode == 2)
								break;
						}
					}
				}
			}
		}
	}

	/* close up all connections, dead or no */
	aim_session_kill(&aimsess); 

	if (faimtest_mode < 2) {
		printf("\n");
		cmd_uninit();
	}

	free(priv.buddyicon);

	/* Get out */
	exit(0);
}

static int conninitdone_admin(aim_session_t *sess, aim_frame_t *fr, ...)
{

	aim_clientready(sess, fr->conn);

	dprintf("initialization done for admin connection\n");

	return 1;
}

static int conninitdone_bos(aim_session_t *sess, aim_frame_t *fr, ...)
{
	struct faimtest_priv *priv = (struct faimtest_priv *)sess->aux_data;
	char buddies[128]; /* this is the new buddy list */
	char profile[256]; /* this is the new profile */ 
	char awaymsg[] = {"blah blah blah Ole! blah blah blah"};

	/* Caution: Buddy1 and Buddy2 are real people! (who I don't know) */
	snprintf(buddies, sizeof(buddies), "Buddy1&Buddy2&%s&", priv->ohcaptainmycaptain ? priv->ohcaptainmycaptain : "blah");
	snprintf(profile, sizeof(profile), "Hello.<br>My captain is %s.  They were dumb enough to leave this message in their client, or they are using faimtest.  Shame on them.", priv->ohcaptainmycaptain);

	aim_reqpersonalinfo(sess, fr->conn);
	aim_bos_reqlocaterights(sess, fr->conn);
	aim_bos_setprofile(sess, fr->conn, profile, awaymsg, AIM_CAPS_BUDDYICON | AIM_CAPS_CHAT | AIM_CAPS_GETFILE | AIM_CAPS_SENDFILE | AIM_CAPS_IMIMAGE | AIM_CAPS_GAMES | AIM_CAPS_SAVESTOCKS | AIM_CAPS_SENDBUDDYLIST | AIM_CAPS_ICQ | AIM_CAPS_ICQUNKNOWN | AIM_CAPS_ICQRTF | AIM_CAPS_ICQSERVERRELAY | AIM_CAPS_TRILLIANCRYPT);
	aim_bos_reqbuddyrights(sess, fr->conn);

	/* send the buddy list and profile (required, even if empty) */
	aim_bos_setbuddylist(sess, fr->conn, buddies);

	aim_reqicbmparams(sess);  

	aim_bos_reqrights(sess, fr->conn);  
	/* set group permissions -- all user classes */
	aim_bos_setgroupperm(sess, fr->conn, AIM_FLAG_ALLUSERS);
	aim_bos_setprivacyflags(sess, fr->conn, AIM_PRIVFLAGS_ALLOWIDLE);

	return 1;
}

static int faimtest_parse_connerr(aim_session_t *sess, aim_frame_t *fr, ...)
{
	struct faimtest_priv *priv = (struct faimtest_priv *)sess->aux_data;
	va_list ap;
	fu16_t code;
	char *msg;

	va_start(ap, fr);
	code = va_arg(ap, int);
	msg = va_arg(ap, char *);
	va_end(ap);

	dvprintf("connerr: Code 0x%04x: %s\n", code, msg);
	aim_conn_kill(sess, &fr->conn); /* this will break the main loop */

	priv->connected = 0;

	return 1;
}

static int faimtest_accountconfirm(aim_session_t *sess, aim_frame_t *fr, ...)
{
	int status;
	va_list ap;

	va_start(ap, fr);
	status = va_arg(ap, int); /* status code of confirmation request */
	va_end(ap);

	dvprintf("account confirmation returned status 0x%04x (%s)\n", status, (status==0x0000)?"email sent":"unknown");

	return 1;
}

#if 0
/* 
 * This kind of function is really not legal in the new bstream way...
 * In fact, clients should never access the aim_frame_t directly in handlers,
 * since that may leave it in a bizare state for the lower layers.  In fact,
 * clients should probably not even get passed a pointer like this.
 *
 */
int faimtest_parse_unknown(aim_session_t *sess, aim_frame_t *fr, ...)
{
	int i;

	aim_bstream_rewind(&fr->data); /* boo! */

	dprintf("\nReceived unknown packet:");
	for (i = 0; aim_bstream_empty(&fr->data); i++) {
		if ((i % 8) == 0)
			dinlineprintf("\n\t");
		dvinlineprintf("0x%2x ", aimbs_get8(&fr->data));
	}
	dinlineprintf("\n\n");

	return 1;
}
#endif

static int faimtest_infochange(aim_session_t *sess, aim_frame_t *fr, ...)
{
	fu16_t change = 0, perms, type;
	int length, str;
	char *val;
	va_list ap;

	va_start(ap, fr);
	change = va_arg(ap, int);
	perms = (fu16_t)va_arg(ap, unsigned int);
	type = (fu16_t)va_arg(ap, unsigned int);
	length = va_arg(ap, int);
	val = va_arg(ap, char *);
	str = va_arg(ap, int);
	va_end(ap);

	dvprintf("info%s: perms = %d, type = %x, length = %d, val = %s\n", change?" change":"", perms, type, length, str?val:"(not string)");

	return 1;
}


static int faimtest_handleredirect(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	struct aim_redirect_data *redir;

	va_start(ap, fr);
	redir = va_arg(ap, struct aim_redirect_data *);

	if (redir->group == 0x0005) {  /* Adverts */
#if 0
		aim_conn_t *tstconn;

		tstconn = aim_newconn(sess, AIM_CONN_TYPE_ADS, ip);
		if (!tstconn || (tstconn->status & AIM_CONN_STATUS_RESOLVERR)) {
			dprintf("faimtest: unable to reconnect with authorizer\n");
		} else {
			aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_FLAPVER, faimtest_flapversion, 0);
			aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
			aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_adverts, 0);
			aim_sendcookie(sess, tstconn, cookie);
			dprintf("sent cookie to adverts host\n");
		}
#endif
	} else if (redir->group == 0x0007) {  /* Authorizer */
		aim_conn_t *tstconn;
		
		tstconn = aim_newconn(sess, AIM_CONN_TYPE_AUTH, redir->ip);
		if (!tstconn || (tstconn->status & AIM_CONN_STATUS_RESOLVERR)) {
			dprintf("unable to reconnect with authorizer\n");
		} else {
			aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_FLAPVER, faimtest_flapversion, 0);
			aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
			aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_admin, 0);
			aim_conn_addhandler(sess, tstconn, 0x0007, 0x0007, faimtest_accountconfirm, 0);
			aim_conn_addhandler(sess, tstconn, 0x0007, 0x0003, faimtest_infochange, 0);
			aim_conn_addhandler(sess, tstconn, 0x0007, 0x0005, faimtest_infochange, 0);
			/* Send the cookie to the Auth */
			aim_sendcookie(sess, tstconn, redir->cookie);
			dprintf("sent cookie to authorizer host\n");
		}
	} else if (redir->group == 0x000d) {  /* ChatNav */

		chatnav_redirect(sess, redir);
		
	} else if (redir->group == 0x000e) { /* Chat */

		chat_redirect(sess, redir);

	} else {
		dvprintf("uh oh... got redirect for unknown service 0x%04x!!\n", redir->group);
	}

	va_end(ap);

	return 1;
}

static int faimtest_icbmparaminfo(aim_session_t *sess, aim_frame_t *fr, ...)
{
	struct aim_icbmparameters *params;
	va_list ap;

	va_start(ap, fr);
	params = va_arg(ap, struct aim_icbmparameters *);
	va_end(ap);

	dvprintf("ICBM Parameters: maxchannel = %d, default flags = 0x%08lx, max msg len = %d, max sender evil = %f, max reciever evil = %f, min msg interval = %ld\n", params->maxchan, params->flags, params->maxmsglen, ((float)params->maxsenderwarn)/10.0, ((float)params->maxrecverwarn)/10.0, params->minmsginterval);

	/*
	 * Set these to your taste, or client medium.  Setting minmsginterval
	 * higher is good for keeping yourself from getting flooded (esp
	 * if you're on a slow connection or something where that would be
	 * useful).
	 */
	params->maxmsglen = 8000;
	params->minmsginterval = 0; /* in milliseconds */

	aim_seticbmparam(sess, params);

	return 1;
}

static int faimtest_parse_buddyrights(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	fu16_t maxbuddies, maxwatchers;

	va_start(ap, fr);
	maxbuddies = va_arg(ap, int);
	maxwatchers = va_arg(ap, int);
	va_end(ap);

	dvprintf("buddy list rights: Max buddies = %d / Max watchers = %d\n", maxbuddies, maxwatchers);

	aim_ssi_reqrights(sess, fr->conn);

	return 1;
}

static int faimtest_bosrights(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	fu16_t maxpermits, maxdenies;

	va_start(ap, fr);
	maxpermits = va_arg(ap, int);
	maxdenies = va_arg(ap, int);
	va_end(ap);

	dvprintf("BOS rights: Max permit = %d / Max deny = %d\n", maxpermits, maxdenies);

	aim_clientready(sess, fr->conn);

	dprintf("officially connected to BOS.\n");

	aim_icq_reqofflinemsgs(sess);

	return 1;
}

static int faimtest_locrights(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	fu16_t maxsiglen;

	va_start(ap, fr);
	maxsiglen = va_arg(ap, int);
	va_end(ap);

	dvprintf("locate rights: max signature length = %d\n", maxsiglen);

	return 1;
}

static int faimtest_reportinterval(aim_session_t *sess, aim_frame_t *fr, ...)
{
	struct faimtest_priv *priv = (struct faimtest_priv *)sess->aux_data;
	va_list ap;
	fu16_t interval;

	va_start(ap, fr);
	interval = va_arg(ap, int);
	va_end(ap);

	dvprintf("minimum report interval: %d (seconds?)\n", interval);

	if (!priv->connected)
		priv->connected++;

#if 0
	aim_reqservice(sess, fr->conn, 0x0005); /* adverts */
	aim_reqservice(sess, fr->conn, 0x000f); /* user directory */

	/* Don't know what this does... */
	/* XXX sess->sn should be normalized by the 0001/000f handler */
	aim_0002_000b(sess, fr->conn, sess->sn);
#endif

	aim_reqicbmparams(sess);

	return 1;
}

static int faimtest_parse_motd(aim_session_t *sess, aim_frame_t *fr, ...)
{
	static char *codes[] = {
		"Unknown",
		"Mandatory upgrade",
		"Advisory upgrade",
		"System bulletin",
		"Top o' the world!"
	};
	static int codeslen = 5;
	char *msg;
	fu16_t id;
	va_list ap;

	va_start(ap, fr);
	id = va_arg(ap, int);
	msg = va_arg(ap, char *);
	va_end(ap);

	dvprintf("motd: %s (%d / %s)\n", msg, id, (id < codeslen)?codes[id]:"unknown");

	return 1;
}

/*
 * This is a little more complicated than it looks.  The module
 * name (proto, boscore, etc) may or may not be given.  If it is
 * not given, then use aim.exe.  If it is given, put ".ocm" on the
 * end of it.
 *
 * Now, if the offset or length requested would cause a read past
 * the end of the file, then the request is considered invalid.  Invalid
 * requests are processed specially.  The value hashed is the
 * the request, put into little-endian (eight bytes: offset followed
 * by length).  
 *
 * Additionally, if the request is valid, the length is mod 4096.  It is
 * important that the length is checked for validity first before doing
 * the mod.
 *
 * Note to Bosco's Brigade: if you'd like to break this, put the 
 * module name on an invalid request.
 *
 */
static int getaimdata(aim_session_t *sess, unsigned char **bufret, int *buflenret, unsigned long offset, unsigned long len, const char *modname)
{
	struct faimtest_priv *priv = (struct faimtest_priv *)sess->aux_data;
	FILE *f;
	static const char defaultmod[] = "aim.exe";
	char *filename = NULL;
	struct stat st;
	unsigned char *buf;
	int invalid = 0;

	if (!bufret || !buflenret)
		return -1;

	if (modname) {

		if (!(filename = malloc(strlen(priv->aimbinarypath)+1+strlen(modname)+4+1))) {
			dperror("memrequest: malloc");
			return -1;
		}

		sprintf(filename, "%s/%s.ocm", priv->aimbinarypath, modname);

	} else {

		if (!(filename = malloc(strlen(priv->aimbinarypath)+1+strlen(defaultmod)+1))) {
			dperror("memrequest: malloc");
			return -1;
		}

		sprintf(filename, "%s/%s", priv->aimbinarypath, defaultmod);

	}

	if (stat(filename, &st) == -1) {
		if (!modname) {
			dperror("memrequest: stat");
			free(filename);
			return -1;
		}
		invalid = 1;
	}

	if (!invalid) {
		if ((offset > st.st_size) || (len > st.st_size))
			invalid = 1;
		else if ((st.st_size - offset) < len)
			len = st.st_size - offset;
		else if ((st.st_size - len) < len)
			len = st.st_size - len;
	}

	if (!invalid && len)
		len %= 4096;

	if (invalid) {
		int i;

		free(filename); /* not needed */

		dvprintf("memrequest: recieved invalid request for 0x%08lx bytes at 0x%08lx (file %s)\n", len, offset, modname);

		i = 8;
		if (modname)
			i += strlen(modname);

		if (!(buf = malloc(i)))
			return -1;

		i = 0;

		if (modname) {
			memcpy(buf, modname, strlen(modname));
			i += strlen(modname);
		}

		/* Damn endianness. This must be little (LSB first) endian. */
		buf[i++] = offset & 0xff;
		buf[i++] = (offset >> 8) & 0xff;
		buf[i++] = (offset >> 16) & 0xff;
		buf[i++] = (offset >> 24) & 0xff;
		buf[i++] = len & 0xff;
		buf[i++] = (len >> 8) & 0xff;
		buf[i++] = (len >> 16) & 0xff;
		buf[i++] = (len >> 24) & 0xff;

		*bufret = buf;
		*buflenret = i;

	} else {

		if (!(buf = malloc(len))) {
			free(filename);
			return -1;
		}

		dvprintf("memrequest: loading %ld bytes from 0x%08lx in \"%s\"...\n", len, offset, filename);

		if (!(f = fopen(filename, "r"))) {
			dperror("memrequest: fopen");
			free(filename);
			free(buf);
			return -1;
		}

		free(filename);

		if (fseek(f, offset, SEEK_SET) == -1) {
			dperror("memrequest: fseek");
			fclose(f);
			free(buf);
			return -1;
		}

		if (fread(buf, len, 1, f) != 1) {
			dperror("memrequest: fread");
			fclose(f);
			free(buf);
			return -1;
		}

		fclose(f);

		*bufret = buf;
		*buflenret = len;

	}

	return 0; /* success! */
}

/*
 * This will get an offset and a length.  The client should read this
 * data out of whatever AIM.EXE binary the user has provided (hopefully
 * it matches the client information thats sent at login) and pass a
 * buffer back to libfaim so it can hash the data and send it to AOL for
 * inspection by the client police.
 */
static int faimtest_memrequest(aim_session_t *sess, aim_frame_t *fr, ...)
{
	struct faimtest_priv *priv = (struct faimtest_priv *)sess->aux_data;
	va_list ap;
	fu32_t offset, len;
	char *modname;
	unsigned char *buf;
	int buflen;

	va_start(ap, fr);
	offset = va_arg(ap, fu32_t);
	len = va_arg(ap, fu32_t);
	modname = va_arg(ap, char *);
	va_end(ap);

	if (priv->aimbinarypath && (getaimdata(sess, &buf, &buflen, offset, len, modname) == 0)) {

		aim_sendmemblock(sess, fr->conn, offset, buflen, buf, AIM_SENDMEMBLOCK_FLAG_ISREQUEST);

		free(buf);

	} else {

		dvprintf("memrequest: unable to use AIM binary (\"%s/%s\"), sending defaults...\n", priv->aimbinarypath, modname);

		aim_sendmemblock(sess, fr->conn, offset, len, NULL, AIM_SENDMEMBLOCK_FLAG_ISREQUEST);

	}

	return 1;
}

static void printuserflags(fu16_t flags)
{

	if (flags & AIM_FLAG_UNCONFIRMED)
		dinlineprintf("UNCONFIRMED ");
	if (flags & AIM_FLAG_ADMINISTRATOR)
		dinlineprintf("ADMINISTRATOR ");
	if (flags & AIM_FLAG_AOL)
		dinlineprintf("AOL ");
	if (flags & AIM_FLAG_OSCAR_PAY)
		dinlineprintf("OSCAR_PAY ");
	if (flags & AIM_FLAG_FREE)
		dinlineprintf("FREE ");
	if (flags & AIM_FLAG_AWAY)
		dinlineprintf("AWAY ");
	if (flags & AIM_FLAG_ICQ)
		dinlineprintf("ICQ ");
	if (flags & AIM_FLAG_WIRELESS)
		dinlineprintf("WIRELESS ");
	if (flags & AIM_FLAG_ACTIVEBUDDY)
		dinlineprintf("ACTIVEBUDDY ");

	return;
}

static int faimtest_handleselfinfo(aim_session_t *sess, aim_frame_t *fr, ...)
{
	aim_userinfo_t *userinfo;

	va_list ap;
	va_start(ap, fr);
	userinfo = va_arg(ap, aim_userinfo_t *);
	va_end(ap);

	dprintf("Self userinfo:\n");
	dvprintf("userinfo: sn: %s\n", userinfo->sn);
	dvprintf("userinfo: warnlevel: %f\n", aim_userinfo_warnlevel(userinfo));
	dvprintf("userinfo: flags: 0x%04x = ", userinfo->flags);
	printuserflags(userinfo->flags);
	dinlineprintf("\n");

	dvprintf("userinfo: membersince: %lu\n", userinfo->membersince);
	dvprintf("userinfo: onlinesince: %lu\n", userinfo->onlinesince);
	dvprintf("userinfo: idletime: 0x%04x\n", userinfo->idletime);
	dvprintf("userinfo: capabilities = %s = 0x%08lx\n", (userinfo->present & AIM_USERINFO_PRESENT_CAPABILITIES) ? "present" : "not present", userinfo->capabilities);

	return 1;
}

static int faimtest_parse_userinfo(aim_session_t *sess, aim_frame_t *fr, ...)
{
	aim_userinfo_t *userinfo;
	char *prof_encoding = NULL;
	char *prof = NULL;
	fu16_t inforeq = 0;

	va_list ap;
	va_start(ap, fr);
	userinfo = va_arg(ap, aim_userinfo_t *);
	inforeq = (fu16_t)va_arg(ap, unsigned int);
	prof_encoding = va_arg(ap, char *);
	prof = va_arg(ap, char *);
	va_end(ap);

	dvprintf("userinfo: sn: %s\n", userinfo->sn);
	dvprintf("userinfo: warnlevel: %f\n", aim_userinfo_warnlevel(userinfo));
	dvprintf("userinfo: flags: 0x%04x = ", userinfo->flags);
	printuserflags(userinfo->flags);
	dinlineprintf("\n");

	dvprintf("userinfo: membersince: %lu\n", userinfo->membersince);
	dvprintf("userinfo: onlinesince: %lu\n", userinfo->onlinesince);
	dvprintf("userinfo: idletime: 0x%04x\n", userinfo->idletime);
	dvprintf("userinfo: capabilities = %s = 0x%08lx\n", (userinfo->present & AIM_USERINFO_PRESENT_CAPABILITIES) ? "present" : "not present", userinfo->capabilities);


	if (inforeq == AIM_GETINFO_GENERALINFO) {

		dvprintf("userinfo: profile_encoding: %s\n", prof_encoding ? prof_encoding : "[none]");
		dvprintf("userinfo: prof: %s\n", prof ? prof : "[none]");

	} else if (inforeq == AIM_GETINFO_AWAYMESSAGE) {

		dvprintf("userinfo: awaymsg_encoding: %s\n", prof_encoding ? prof_encoding : "[none]");
		dvprintf("userinfo: awaymsg: %s\n", prof ? prof : "[none]");

	} else if (inforeq == AIM_GETINFO_CAPABILITIES) {

		dprintf("userinfo: capabilities: see above\n");

	} else 
		dprintf("userinfo: unknown info request\n");

	return 1;
}

static int faimtest_handlecmd(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, const char *tmpstr)
{
	struct faimtest_priv *priv = (struct faimtest_priv *)sess->aux_data;

	if (!strncmp(tmpstr, "disconnect", 10)) {

		logout(sess);

	} else if (strstr(tmpstr, "goodday")) {

		aim_send_im(sess, userinfo->sn, AIM_IMFLAGS_ACK, "Good day to you too.");

	} else if (strstr(tmpstr, "haveicon") && priv->buddyicon) {
		struct aim_sendimext_args args;
		static const char iconmsg[] = {"I have an icon"};

		args.destsn = userinfo->sn;
		args.flags = AIM_IMFLAGS_HASICON;
		args.msg = iconmsg;
		args.msglen = strlen(iconmsg);
		args.iconlen = priv->buddyiconlen;
		args.iconstamp = priv->buddyiconstamp;
		args.iconsum = priv->buddyiconsum;

		aim_send_im_ext(sess, &args);

	} else if (strstr(tmpstr, "sendbin")) {
		struct aim_sendimext_args args;
		static const unsigned char data[] = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
			0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
			0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
			0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		};

		/*
		 * I put this here as a demonstration of how to send 
		 * arbitrary binary data via OSCAR ICBM's without the need
		 * for escape or baseN encoding of any sort.  
		 *
		 * Apparently if you set the charset to something WinAIM
		 * doesn't recognize, it will completly ignore the message.
		 * That is, it will not display anything in the conversation
		 * window for the user that recieved it.
		 *
		 * HOWEVER, if they do not have a conversation window open
		 * for you, a new one will be created, but it will not have
		 * any messages in it.  Therefore sending these things could
		 * be a great way to seemingly subliminally convince people
		 * to talk to you...
		 *
		 */
		args.destsn = userinfo->sn;
		args.flags = AIM_IMFLAGS_CUSTOMCHARSET;
		args.charset = args.charsubset = 0x4242;
		args.msg = data;
		args.msglen = sizeof(data);

		aim_send_im_ext(sess, &args);

	} else if (strstr(tmpstr, "sendmulti")) {
		struct aim_sendimext_args args;
		aim_mpmsg_t mpm;
		static const fu16_t unidata[] = { /* "UNICODE." */
			0x0055, 0x004e, 0x0049, 0x0043,
			0x004f, 0x0044, 0x0045, 0x002e,
		};
		static const int unidatalen = 8;

		/*
		 * This is how multipart messages should be sent.
		 *
		 * This should render as:
		 *        "Part 1, ASCII.  UNICODE.Part 3, ASCII.  "
		 */

		aim_mpmsg_init(sess, &mpm);

		aim_mpmsg_addascii(sess, &mpm, "Part 1, ASCII.  ");
		aim_mpmsg_addunicode(sess, &mpm, unidata, unidatalen);
		aim_mpmsg_addascii(sess, &mpm, "Part 3, ASCII.  ");

		args.destsn = userinfo->sn;
		args.flags = AIM_IMFLAGS_MULTIPART;
		args.mpmsg = &mpm;

		aim_send_im_ext(sess, &args);

		aim_mpmsg_free(sess, &mpm);

	} else if (strstr(tmpstr, "sendprebin")) {
		static const unsigned char data[] = {
			0x00, 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07,
			0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0x0f,
			0x10, 0x11, 0x12, 0x13, 0x14, 0x15, 0x16, 0x17,
			0x18, 0x19, 0x1a, 0x1b, 0x1c, 0x1d, 0x1e, 0x1f,
			0x20, 0x21, 0x22, 0x23, 0x24, 0x25, 0x26, 0x27,
			0x28, 0x29, 0x2a, 0x2b, 0x2c, 0x2d, 0x2e, 0x2f,
		};
		struct aim_sendimext_args args;
		aim_mpmsg_t mpm;

		/*
		 * This demonstrates sending a human-readable preamble,
		 * and then arbitrary binary data.
		 *
		 * This means that you can very inconspicuously send binary
		 * attachments to other users.  In WinAIM, this appears as
		 * though it only had the ASCII portion.
		 *
		 */

		aim_mpmsg_init(sess, &mpm);

		aim_mpmsg_addascii(sess, &mpm, "This message has binary data.");
		aim_mpmsg_addraw(sess, &mpm, 0x4242, 0x4242, data, sizeof(data));

		args.destsn = userinfo->sn;
		args.flags = AIM_IMFLAGS_MULTIPART;
		args.mpmsg = &mpm;

		aim_send_im_ext(sess, &args);

		aim_mpmsg_free(sess, &mpm);

	} else if (strstr(tmpstr, "havefeat")) {
		struct aim_sendimext_args args;
		static const char featmsg[] = {"I have nifty features."};
		fu8_t features[] = {0x01, 0x01, 0x01, 0x02, 0x42, 0x43, 0x44, 0x45};

		args.destsn = userinfo->sn;
		args.flags = AIM_IMFLAGS_CUSTOMFEATURES;
		args.msg = featmsg;
		args.msglen = strlen(featmsg);
		args.features = features;
		args.featureslen = sizeof(features);

		aim_send_im_ext(sess, &args);

	} else if (strstr(tmpstr, "sendicon") && priv->buddyicon) {

		aim_send_icon(sess, userinfo->sn, priv->buddyicon, priv->buddyiconlen, priv->buddyiconstamp, priv->buddyiconsum);

	} else if (strstr(tmpstr, "warnme")) {

		dprintf("icbm: sending non-anon warning\n");
		aim_send_warning(sess, conn, userinfo->sn, 0);

	} else if (strstr(tmpstr, "anonwarn")) {

		dprintf("icbm: sending anon warning\n");
		aim_send_warning(sess, conn, userinfo->sn, AIM_WARN_ANON);

	} else if (strstr(tmpstr, "setdirectoryinfo")) {

		dprintf("icbm: sending backwards profile data\n");
		aim_setdirectoryinfo(sess, conn, "tsrif", "elddim", "tsal", "nediam", "emankcin", "teerts", "ytic", "etats", "piz", 0, 1);

	} else if (strstr(tmpstr, "setinterests")) {

		dprintf("icbm: setting fun interests\n");
		aim_setuserinterests(sess, conn, "interest1", "interest2", "interest3", "interest4", "interest5", 1);

	} else if (!strncmp(tmpstr, "getfile", 7)) {

		if (!priv->ohcaptainmycaptain) {

			aim_send_im(sess, userinfo->sn, AIM_IMFLAGS_ACK, "I have no owner!");

		} else 
			getfile_start(sess, conn, (strlen(tmpstr) < 8)?priv->ohcaptainmycaptain:tmpstr+8);
		
	} else if (!strncmp(tmpstr, "open chatnav", 12)) {

		aim_reqservice(sess, conn, AIM_CONN_TYPE_CHATNAV);

	} else if (!strncmp(tmpstr, "create", 6)) {

		aim_chatnav_createroom(sess,aim_getconn_type(sess, AIM_CONN_TYPE_CHATNAV), (strlen(tmpstr) < 7)?"WorldDomination":tmpstr+7, 0x0004);

	} else if (!strncmp(tmpstr, "close chatnav", 13)) {
		aim_conn_t *chatnavconn;

		if ((chatnavconn = aim_getconn_type(sess, AIM_CONN_TYPE_CHATNAV)))
			aim_conn_kill(sess, &chatnavconn);

	} else if (!strncmp(tmpstr, "join", 4)) {

		aim_chat_join(sess, conn, 0x0004, "worlddomination", 0x0000);

	} else if (!strncmp(tmpstr, "leave", 5)) {

		aim_chat_leaveroom(sess, "worlddomination");

	} else if (!strncmp(tmpstr, "getinfo", 7)) {

#if 0
		aim_getinfo(sess, conn, "75784102", AIM_GETINFO_GENERALINFO);
		aim_getinfo(sess, conn, "15853637", AIM_GETINFO_AWAYMESSAGE);
#endif
		aim_getinfo(sess, conn, "midendian", AIM_GETINFO_GENERALINFO);
		aim_getinfo(sess, conn, "midendian", AIM_GETINFO_AWAYMESSAGE);
		aim_getinfo(sess, conn, "midendian", AIM_GETINFO_CAPABILITIES);

	} else if (strstr(tmpstr, "open directim")) {

		directim_start(sess, userinfo->sn);

	} else if(strstr(tmpstr, "lookup")) {

		aim_usersearch_address(sess, conn, "mid@auk.cx");

	} else if (!strncmp(tmpstr, "reqsendmsg", 10)) {

		aim_send_im(sess, priv->ohcaptainmycaptain, 0, "sendmsg 7900");

	} else if (!strncmp(tmpstr, "reqadmin", 8)) {

		aim_reqservice(sess, conn, AIM_CONN_TYPE_AUTH);

	} else if (!strncmp(tmpstr, "changenick", 10)) {

		aim_admin_setnick(sess, aim_getconn_type(sess, AIM_CONN_TYPE_AUTH), "diputs8  1");

	} else if (!strncmp(tmpstr, "reqconfirm", 10)) {

		aim_admin_reqconfirm(sess, aim_getconn_type(sess, AIM_CONN_TYPE_AUTH));

	} else if (!strncmp(tmpstr, "reqemail", 8)) {

		aim_admin_getinfo(sess, aim_getconn_type(sess, AIM_CONN_TYPE_AUTH), 0x0011);

	} else if (!strncmp(tmpstr, "changepass", 8)) {

		aim_admin_changepasswd(sess, aim_getconn_type(sess, AIM_CONN_TYPE_AUTH), "NEWPASSWORD", "OLDPASSWORD");

	} else if (!strncmp(tmpstr, "setemail", 8)) {

		aim_admin_setemail(sess, aim_getconn_type(sess, AIM_CONN_TYPE_AUTH), "NEWEMAILADDRESS");

	} else if (!strncmp(tmpstr, "sendmsg", 7)) {
		int i;

		i = atoi(tmpstr+8);
		if (i < 10000) {
			char *newbuf;
			int z;

			newbuf = malloc(i+1);
			for (z = 0; z < i; z++)
				newbuf[z] = (z % 10)+0x30;
			newbuf[i] = '\0';
			dvprintf("sending message to '%s': '%s'\n", userinfo->sn, newbuf);
			aim_send_im(sess, userinfo->sn, 0 /*AIM_IMFLAGS_ACK | AIM_IMFLAGS_AWAY*/, newbuf);
			free(newbuf);
		}

	} else if (strstr(tmpstr, "seticqstatus")) {

		aim_setextstatus(sess, conn, AIM_ICQ_STATE_DND);

	} else if (strstr(tmpstr, "rtfmsg")) {
		static const char rtfmsg[] = {"{\\rtf1\\ansi\\ansicpg1252\\deff0\\deflang1033{\\fonttbl{\\f0\\fswiss\\fcharset0 Arial;}{\\f1\\froman\\fprq2\\fcharset0 Georgia;}{\\f2\\fmodern\\fprq1\\fcharset0 MS Mincho;}{\\f3\\froman\\fprq2\\fcharset2 Symbol;}}\\viewkind4\\uc1\\pard\\f0\\fs20 Test\\f1 test\\f2\fs44 test\\f3\\fs20 test\\f0\\par}"};
		struct aim_sendrtfmsg_args rtfargs;

		memset(&rtfargs, 0, sizeof(rtfargs));

		rtfargs.destsn = userinfo->sn;
		rtfargs.fgcolor = 0xffffffff;
		rtfargs.bgcolor = 0x00000000;
		rtfargs.rtfmsg = rtfmsg;
		aim_send_rtfmsg(sess, &rtfargs);

	} else {

		dprintf("unknown command.\n");
		aim_add_buddy(sess, conn, userinfo->sn);

	}  

	return 0;
}

/*
 * Channel 1: Standard Message
 */
static int faimtest_parse_incoming_im_chan1(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch1_args *args)
{
	struct faimtest_priv *priv = (struct faimtest_priv *)sess->aux_data;
	char *tmpstr;
	int clienttype = AIM_CLIENTTYPE_UNKNOWN;
	char realmsg[8192+1] = {""};
	clienttype = aim_fingerprintclient(args->features, args->featureslen);

	dvprintf("icbm: sn = \"%s\"\n", userinfo->sn);
	dvprintf("icbm: probable client type: %d\n", clienttype);
	dvprintf("icbm: warnlevel = %f\n", aim_userinfo_warnlevel(userinfo));
	dvprintf("icbm: flags = 0x%04x = ", userinfo->flags);
	printuserflags(userinfo->flags);
	dinlineprintf("\n");

	dvprintf("icbm: membersince = %lu\n", userinfo->membersince);
	dvprintf("icbm: onlinesince = %lu\n", userinfo->onlinesince);
	dvprintf("icbm: idletime = 0x%04x\n", userinfo->idletime);
	dvprintf("icbm: capabilities = %s = 0x%08lx\n", (userinfo->present & AIM_USERINFO_PRESENT_CAPABILITIES) ? "present" : "not present", userinfo->capabilities);

	dprintf("icbm: icbmflags = ");
	if (args->icbmflags & AIM_IMFLAGS_AWAY)
		dinlineprintf("away ");
	if (args->icbmflags & AIM_IMFLAGS_ACK)
		dinlineprintf("ackrequest ");
	if (args->icbmflags & AIM_IMFLAGS_OFFLINE)
		dinlineprintf("offline ");
	if (args->icbmflags & AIM_IMFLAGS_BUDDYREQ)
		dinlineprintf("buddyreq ");
	if (args->icbmflags & AIM_IMFLAGS_HASICON)
		dinlineprintf("hasicon ");
	dinlineprintf("\n");

	if (args->icbmflags & AIM_IMFLAGS_CUSTOMCHARSET)
		dvprintf("icbm: encoding flags = {%04x, %04x}\n", args->charset, args->charsubset);

	/*
	 * Quickly convert it to eight bit format, replacing non-ASCII UNICODE 
	 * characters with their equivelent HTML entity.
	 */
	if (args->icbmflags & AIM_IMFLAGS_UNICODE) {
		int i;

		for (i = 0; i < args->msglen; i += 2) {
			fu16_t uni;

			uni = ((args->msg[i] & 0xff) << 8) | (args->msg[i+1] & 0xff);

			if ((uni < 128) || ((uni >= 160) && (uni <= 255))) { /* ISO 8859-1 */

				snprintf(realmsg+strlen(realmsg), sizeof(realmsg)-strlen(realmsg), "%c", uni);

			} else { /* something else, do UNICODE entity */

				snprintf(realmsg+strlen(realmsg), sizeof(realmsg)-strlen(realmsg), "&#%04x;", uni);

			}

		}

	} else {

		/*
		 * For non-UNICODE encodings (ASCII and ISO 8859-1), there is 
		 * no need to do anything special here.  Most 
		 * terminals/whatever will be able to display such characters 
		 * unmodified.
		 *
		 * Beware that PC-ASCII 128 through 159 are _not_ actually 
		 * defined in ASCII or ISO 8859-1, and you should send them as 
		 * UNICODE.  WinAIM will send these characters in a UNICODE 
		 * message, so you need to do so as well.
		 *
		 * You may not think it necessary to handle UNICODE messages.  
		 * You're probably wrong.  For one thing, Microsoft "Smart
		 * Quotes" will be sent by WinAIM as UNICODE (not HTML UNICODE,
		 * but real UNICODE). If you don't parse UNICODE at all, your 
		 * users will get a blank message instead of the message 
		 * containing Smart Quotes.
		 *
		 */
		strncpy(realmsg, args->msg, sizeof(realmsg));
	}

	dvprintf("icbm: message: %s\n", realmsg);

	if (args->icbmflags & AIM_IMFLAGS_MULTIPART) {
		aim_mpmsg_section_t *sec;
		int z;

		dvprintf("icbm: multipart: this message has %d parts\n", args->mpmsg.numparts);

		for (sec = args->mpmsg.parts, z = 0; sec; sec = sec->next, z++) {
			if ((sec->charset == 0x0000) || (sec->charset == 0x0003) || (sec->charset == 0xffff)) {
				dvprintf("icbm: multipart:   part %d: charset 0x%04x, subset 0x%04x, msg = %s\n", z, sec->charset, sec->charsubset, sec->data);
			} else {
				dvprintf("icbm: multipart:   part %d: charset 0x%04x, subset 0x%04x, binary or UNICODE data\n", z, sec->charset, sec->charsubset);
			}
		}
	}

	if (args->icbmflags & AIM_IMFLAGS_HASICON) {
		aim_send_im(sess, userinfo->sn, AIM_IMFLAGS_BUDDYREQ, "You have an icon");
		dvprintf("icbm: their icon: iconstamp = %ld, iconlen = 0x%08lx, iconsum = 0x%04x\n", args->iconstamp, args->iconlen, args->iconsum);
	}

	if (realmsg) {
		int i = 0;

		while (realmsg[i] == '<') {
			if (realmsg[i] == '<') {
				while (realmsg[i] != '>')
					i++;
				i++;
			}
		}
		tmpstr = realmsg+i;

		faimtest_handlecmd(sess, conn, userinfo, tmpstr);

	}

	if (priv->buddyicon && (args->icbmflags & AIM_IMFLAGS_BUDDYREQ))
		aim_send_icon(sess, userinfo->sn, priv->buddyicon, priv->buddyiconlen, priv->buddyiconstamp, priv->buddyiconsum);

	return 1;
}

/*
 * Channel 2: Rendevous Request
 */
static int faimtest_parse_incoming_im_chan2(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args)
{

	dvprintf("rendezvous: source sn = %s\n", userinfo->sn);
	dvprintf("rendezvous: \twarnlevel = %f\n", aim_userinfo_warnlevel(userinfo));
	dvprintf("rendezvous: \tclass = 0x%04x = ", userinfo->flags);
	printuserflags(userinfo->flags);
	dinlineprintf("\n");

	dvprintf("rendezvous: \tonlinesince = %lu\n", userinfo->onlinesince);
	dvprintf("rendezvous: \tidletime = 0x%04x\n", userinfo->idletime);

	dvprintf("rendezvous: message/description = %s\n", args->msg);
	dvprintf("rendezvous: encoding = %s\n", args->encoding);
	dvprintf("rendezvous: language = %s\n", args->language);

	if (args->reqclass == AIM_CAPS_GETFILE) {
		
		getfile_requested(sess, conn, userinfo, args);
		
	} else if (args->reqclass == AIM_CAPS_SENDFILE) {

		dprintf("send file!\n");

	} else if (args->reqclass == AIM_CAPS_CHAT) {

		dvprintf("chat invitation: room name = %s\n", args->info.chat.roominfo.name);
		dvprintf("chat invitation: exchange = 0x%04x\n", args->info.chat.roominfo.exchange);
		dvprintf("chat invitation: instance = 0x%04x\n", args->info.chat.roominfo.instance);

		/* Automatically join room... */
		dvprintf("chat invitiation: autojoining %s...\n", args->info.chat.roominfo.name);
		aim_chat_join(sess, conn, args->info.chat.roominfo.exchange, args->info.chat.roominfo.name, args->info.chat.roominfo.instance);

	} else if (args->reqclass == AIM_CAPS_IMIMAGE) {

		directim_requested(sess, conn, userinfo, args);

	} else if (args->reqclass == AIM_CAPS_BUDDYICON) {

		dvprintf("Buddy Icon from %s, length = %lu\n", userinfo->sn, args->info.icon.length);

	} else if (args->reqclass == AIM_CAPS_ICQRTF) {

		dvprintf("RTF message from %s: (fgcolor = 0x%08lx, bgcolor = 0x%08lx) %s\n", userinfo->sn, args->info.rtfmsg.fgcolor, args->info.rtfmsg.bgcolor, args->info.rtfmsg.rtfmsg);

	} else {

		dvprintf("icbm: unknown reqclass (%d)\n", args->reqclass);
	}

	return 1;
}

static int faimtest_parse_incoming_im(aim_session_t *sess, aim_frame_t *fr, ...)
{
	fu16_t channel;
	aim_userinfo_t *userinfo;
	va_list ap;
	int ret = 0;

	va_start(ap, fr);
	channel = (fu16_t)va_arg(ap, unsigned int);
	userinfo = va_arg(ap, aim_userinfo_t *);

	if (channel == 1) {
		struct aim_incomingim_ch1_args *args;

		args = va_arg(ap, struct aim_incomingim_ch1_args *);

		ret = faimtest_parse_incoming_im_chan1(sess, fr->conn, userinfo, args);

	} else if (channel == 2) {
		struct aim_incomingim_ch2_args *args;

		args = va_arg(ap, struct aim_incomingim_ch2_args *);

		ret = faimtest_parse_incoming_im_chan2(sess, fr->conn, userinfo, args);
	} else
		dvprintf("unsupported channel 0x%04x\n", channel);

	va_end(ap);

	dvprintf("icbm: done with ICBM handling (ret = %d)\n", ret);

	return 1;
}

static int faimtest_parse_oncoming(aim_session_t *sess, aim_frame_t *fr, ...)
{
	aim_userinfo_t *userinfo;

	va_list ap;
	va_start(ap, fr);
	userinfo = va_arg(ap, aim_userinfo_t *);
	va_end(ap);

	dvprintf("%ld  %s is now online (flags: %04x = %s%s%s%s%s%s%s%s) (caps = %s = 0x%08lx)\n",
			time(NULL),
			userinfo->sn, userinfo->flags,
			(userinfo->flags&AIM_FLAG_UNCONFIRMED)?" UNCONFIRMED":"",
			(userinfo->flags&AIM_FLAG_ADMINISTRATOR)?" ADMINISTRATOR":"",
			(userinfo->flags&AIM_FLAG_AOL)?" AOL":"",
			(userinfo->flags&AIM_FLAG_OSCAR_PAY)?" OSCAR_PAY":"",
			(userinfo->flags&AIM_FLAG_FREE)?" FREE":"",
			(userinfo->flags&AIM_FLAG_AWAY)?" AWAY":"",
			(userinfo->flags&AIM_FLAG_ICQ)?" ICQ":"",
			(userinfo->flags&AIM_FLAG_WIRELESS)?" WIRELESS":"",
			(userinfo->present & AIM_USERINFO_PRESENT_CAPABILITIES) ? "present" : "not present",
			userinfo->capabilities);
	return 1;
}

static int faimtest_parse_offgoing(aim_session_t *sess, aim_frame_t *fr, ...)
{
	aim_userinfo_t *userinfo;
	va_list ap;
	
	va_start(ap, fr);
	userinfo = va_arg(ap, aim_userinfo_t *);
	va_end(ap);

	dvprintf("%ld  %s is now offline (flags: %04x = %s%s%s%s%s%s%s%s) (caps = %s = 0x%08lx)\n",
			 time(NULL),
			 userinfo->sn, userinfo->flags,
			 (userinfo->flags&AIM_FLAG_UNCONFIRMED)?" UNCONFIRMED":"",
			 (userinfo->flags&AIM_FLAG_ADMINISTRATOR)?" ADMINISTRATOR":"",
			 (userinfo->flags&AIM_FLAG_AOL)?" AOL":"",
			 (userinfo->flags&AIM_FLAG_OSCAR_PAY)?" OSCAR_PAY":"",
			 (userinfo->flags&AIM_FLAG_FREE)?" FREE":"",
			 (userinfo->flags&AIM_FLAG_AWAY)?" AWAY":"",
			 (userinfo->flags&AIM_FLAG_ICQ)?" ICQ":"",
			 (userinfo->flags&AIM_FLAG_WIRELESS)?" WIRELESS":"",
			 (userinfo->present & AIM_USERINFO_PRESENT_CAPABILITIES) ? "present" : "not present",
			 userinfo->capabilities);

	return 1;
}

/* Used by chat as well. */
int faimtest_parse_genericerr(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	fu16_t reason;

	va_start(ap, fr);
	reason = (fu16_t)va_arg(ap, unsigned int);
	va_end(ap);

	dvprintf("snac threw error (reason 0x%04x: %s)\n", reason, (reason<msgerrreasonslen)?msgerrreasons[reason]:"unknown");

	return 1;
}

static int faimtest_parse_msgerr(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	char *destsn;
	fu16_t reason;

	va_start(ap, fr);
	reason = (fu16_t)va_arg(ap, unsigned int);
	destsn = va_arg(ap, char *);
	va_end(ap);

	dvprintf("message to %s bounced (reason 0x%04x: %s)\n", destsn, reason, (reason<msgerrreasonslen)?msgerrreasons[reason]:"unknown");

	return 1;
}

static int faimtest_parse_locerr(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	char *destsn;
	fu16_t reason;

	va_start(ap, fr);
	reason = (fu16_t)va_arg(ap, unsigned int);
	destsn = va_arg(ap, char *);
	va_end(ap);

	dvprintf("user information for %s unavailable (reason 0x%04x: %s)\n", destsn, reason, (reason<msgerrreasonslen)?msgerrreasons[reason]:"unknown");

	return 1;
}

static int faimtest_parse_misses(aim_session_t *sess, aim_frame_t *fr, ...)
{
	static char *missedreasons[] = {
		"Invalid (0)",
		"Message too large",
		"Rate exceeded",
		"Evil Sender",
		"Evil Receiver"
	};
	static int missedreasonslen = 5;

	va_list ap;
	fu16_t chan, nummissed, reason;
	aim_userinfo_t *userinfo;

	va_start(ap, fr);
	chan = (fu16_t)va_arg(ap, unsigned int);
	userinfo = va_arg(ap, aim_userinfo_t *);
	nummissed = (fu16_t)va_arg(ap, unsigned int);
	reason = (fu16_t)va_arg(ap, unsigned int);
	va_end(ap);

	dvprintf("missed %d messages from %s on channel %d (reason %d: %s)\n", nummissed, userinfo->sn, chan, reason, (reason<missedreasonslen)?missedreasons[reason]:"unknown");

	return 1;
}

/*
 * Received in response to an IM sent with the AIM_IMFLAGS_ACK option.
 */
static int faimtest_parse_msgack(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	fu16_t type;
	char *sn = NULL;

	va_start(ap, fr);
	type = (fu16_t)va_arg(ap, unsigned int);
	sn = va_arg(ap, char *);
	va_end(ap);

	dvprintf("msgack: 0x%04x / %s\n", type, sn);

	return 1;
}

static int faimtest_parse_ratechange(aim_session_t *sess, aim_frame_t *fr, ...)
{
	static char *codes[5] = {
		"invalid",
		"change",
		"warning",
		"limit",
		"limit cleared"
	};
	va_list ap;
	fu16_t code, rateclass;
	fu32_t windowsize, clear, alert, limit, disconnect;
	fu32_t currentavg, maxavg;

	va_start(ap, fr); 

	/* See code explanations below */
	code = (fu16_t)va_arg(ap, unsigned int);

	/*
	 * See comments above aim_parse_ratechange_middle() in aim_rxhandlers.c.
	 */
	rateclass = (fu16_t)va_arg(ap, unsigned int);

	/*
	 * Not sure what this is exactly.  I think its the temporal 
	 * relation factor (ie, how to make the rest of the numbers
	 * make sense in the real world). 
	 */
	windowsize = va_arg(ap, fu32_t);

	/* Explained below */
	clear = va_arg(ap, fu32_t);
	alert = va_arg(ap, fu32_t);
	limit = va_arg(ap, fu32_t);
	disconnect = va_arg(ap, fu32_t);
	currentavg = va_arg(ap, fu32_t);
	maxavg = va_arg(ap, fu32_t);

	va_end(ap);


	dvprintf("rate %s (rate class 0x%04x): curavg = %ld, maxavg = %ld, alert at %ld, clear warning at %ld, limit at %ld, disconnect at %ld (window size = %ld)\n",
				(code < 5)?codes[code]:"invalid",
				rateclass,
				currentavg, maxavg,
				alert, clear,
				limit, disconnect,
				windowsize);
#if 0
	if (code == AIM_RATE_CODE_CHANGE) {
		/*
		 * Not real sure when these get sent.
		 */
		if (currentavg >= clear)
			aim_conn_setlatency(fr->conn, 0);

	} else if (code == AIM_RATE_CODE_WARNING) {
		/*
		 * We start getting WARNINGs the first time we go below the 
		 * 'alert' limit (currentavg < alert) and they stop when 
		 * either we pause long enough for currentavg to go above 
		 * 'clear', or until we flood it bad enough to go below 
		 * 'limit' (and start getting LIMITs instead) or even further 
		 * and go below 'disconnect' and get disconnected completely 
		 * (and won't be able to login right away either).
		 */
		aim_conn_setlatency(fr->conn, windowsize/4); /* XXX this is bogus! */ 

	} else if (code == AIM_RATE_CODE_LIMIT) {
		/*
		 * When we hit LIMIT, messages will start getting dropped.
		 */
		aim_conn_setlatency(fr->conn, windowsize/2); /* XXX this is bogus! */ 

	} else if (code == AIM_RATE_CODE_CLEARLIMIT) {
		/*
		 * The limit is cleared when curavg goes above 'clear'.
		 */
		aim_conn_setlatency(fr->conn, 0); 
	}
#endif
	return 1;
}

static int faimtest_parse_evilnotify(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	fu16_t newevil;
	aim_userinfo_t *userinfo;

	va_start(ap, fr);
	newevil = (fu16_t)va_arg(ap, unsigned int);
	userinfo = va_arg(ap, aim_userinfo_t *);
	va_end(ap);

	/*
	 * Evil Notifications that are lacking userinfo->sn are anon-warns
	 * if they are an evil increases, but are not warnings at all if its
	 * a decrease (its the natural backoff happening).
	 *
	 * newevil is passed as an int representing the new evil value times
	 * ten.
	 */
	dvprintf("evil level change: new value = %2.1f%% (caused by %s)\n", ((float)newevil)/10, (userinfo && strlen(userinfo->sn))?userinfo->sn:"anonymous");

	return 1;
}

static int faimtest_parse_searchreply(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	char *address, *SNs;
	int i, num;

	va_start(ap, fr);
	address = va_arg(ap, char *);
	num = va_arg(ap, int);
	SNs = va_arg(ap, char *);
	va_end(ap);

	dvprintf("E-Mail Search Results for %s: ", address);

	for(i = 0; i < num; i++)
		dvinlineprintf("%s, ", &SNs[i*(MAXSNLEN+1)]);
	dinlineprintf("\n");

	return 1;
}

static int faimtest_parse_searcherror(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	char *address;

	va_start(ap, fr);
	address = va_arg(ap, char *);
	va_end(ap);

	dvprintf("E-Mail Search Results for %s: No Results or Invalid Email\n", address);

	return 1;
}

static int handlepopup(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	char *msg, *url;
	fu16_t width, height, delay;

	va_start(ap, fr);
	msg = va_arg(ap, char *);
	url = va_arg(ap, char *);
	width = va_arg(ap, unsigned int);
	height = va_arg(ap, unsigned int);
	delay = va_arg(ap, unsigned int);
	va_end(ap);

	dvprintf("popup: (%dx%x:%d) %s (%s)\n", width, height, delay, msg, url);

	return 1;
}

static int serverpause(aim_session_t *sess, aim_frame_t *fr, ...)
{

	aim_sendpauseack(sess, fr->conn);

	return 1;
}

static int migrate(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_conn_t *bosconn;
	char *bosip;
	fu8_t *cookie;

	va_start(ap, fr);
	bosip = va_arg(ap, char *);
	cookie = va_arg(ap, fu8_t *);
	va_end(ap);

	dvprintf("migration in progress -- new BOS is %s -- disconnecting\n", bosip);
	aim_conn_kill(sess, &fr->conn);

	if (!(bosconn = aim_newconn(sess, AIM_CONN_TYPE_BOS, bosip))) {
		dprintf("migrate: could not connect to BOS: internal error\n");
		return 1;
	} else if (bosconn->status & AIM_CONN_STATUS_CONNERR) {	
		dprintf("migrate: could not connect to BOS\n");
		aim_conn_kill(sess, &bosconn);
		return 1;
	}

	/* Login will happen all over again. */
	addcb_bos(sess, bosconn);

	aim_sendcookie(sess, bosconn, cookie);

	return 1;
}

static int ssirights(aim_session_t *sess, aim_frame_t *fr, ...)
{

	dprintf("got SSI rights, requesting data\n");

	aim_ssi_reqdata(sess, fr->conn, 0, 0x0000);

	return 1;
}

static int ssidata(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	fu8_t fmtver;
	fu16_t itemcount;
	fu32_t stamp;
	struct aim_ssi_item *list, *l;

	va_start(ap, fr);
	fmtver = va_arg(ap, unsigned int);
	itemcount = va_arg(ap, unsigned int);
	stamp = va_arg(ap, fu32_t);
	list = va_arg(ap, struct aim_ssi_item *);
	va_end(ap);

	dvprintf("got SSI data: (0x%02x, %d items, %ld)\n", fmtver, itemcount, stamp);
	for (l = list; l; l = l->next) {
		dvprintf("\t0x%04x (%s) - 0x%04x/0x%04x\n",
				l->type,
				l->name,
				l->gid, l->bid);
	}

	aim_ssi_enable(sess, fr->conn);

	return 1;
}

static int ssidatanochange(aim_session_t *sess, aim_frame_t *fr, ...)
{

	dprintf("server says we have the latest SSI data already\n");

	aim_ssi_enable(sess, fr->conn);

	return 1;
}

static int offlinemsg(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	struct aim_icq_offlinemsg *msg;

	va_start(ap, fr);
	msg = va_arg(ap, struct aim_icq_offlinemsg *);
	va_end(ap);

	if (msg->type == 0x0001) {

		dvprintf("offline message from %ld at %d/%d/%d %02d:%02d : %s\n", msg->sender, msg->year, msg->month, msg->day, msg->hour, msg->minute, msg->msg);

	} else {
		dvprintf("unknown offline message type 0x%04x\n", msg->type);
	}

	return 1;
}

static int offlinemsgdone(aim_session_t *sess, aim_frame_t *fr, ...)
{

	/* Tell the server to delete them. */
	aim_icq_ackofflinemsgs(sess);

	return 1;
}

void addcb_bos(aim_session_t *sess, aim_conn_t *bosconn)
{

	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_bos, 0);

	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_SELFINFO, faimtest_handleselfinfo, 0);
	aim_conn_addhandler(sess, bosconn, 0x0013, 0x0003, ssirights, 0);
	aim_conn_addhandler(sess, bosconn, 0x0013, 0x0006, ssidata, 0);
	aim_conn_addhandler(sess, bosconn, 0x0013, 0x000f, ssidatanochange, 0);
	aim_conn_addhandler(sess, bosconn, 0x0008, 0x0002, handlepopup, 0);
	aim_conn_addhandler(sess, bosconn, 0x0009, 0x0003, faimtest_bosrights, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_REDIRECT, faimtest_handleredirect, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_STS, AIM_CB_STS_SETREPORTINTERVAL, faimtest_reportinterval, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_RIGHTSINFO, faimtest_parse_buddyrights, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_MOTD, faimtest_parse_motd, 0);
	aim_conn_addhandler(sess, bosconn, 0x0004, 0x0005, faimtest_icbmparaminfo, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR, faimtest_parse_connerr, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_RIGHTSINFO, faimtest_locrights, 0);
	aim_conn_addhandler(sess, bosconn, 0x0001, 0x001f, faimtest_memrequest, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_ONCOMING, faimtest_parse_oncoming, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_BUD, AIM_CB_BUD_OFFGOING, faimtest_parse_offgoing, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_INCOMING, faimtest_parse_incoming_im, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_ERROR, faimtest_parse_locerr, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_MISSEDCALL, faimtest_parse_misses, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_RATECHANGE, faimtest_parse_ratechange, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_GEN, AIM_CB_GEN_EVIL, faimtest_parse_evilnotify, 0);
	aim_conn_addhandler(sess, bosconn, 0x000a, 0x0001, faimtest_parse_searcherror, 0);
	aim_conn_addhandler(sess, bosconn, 0x000a, 0x0003, faimtest_parse_searchreply, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_ERROR, faimtest_parse_msgerr, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_LOC, AIM_CB_LOC_USERINFO, faimtest_parse_userinfo, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_MSG, AIM_CB_MSG_ACK, faimtest_parse_msgack, 0);

	aim_conn_addhandler(sess, bosconn, 0x0001, 0x0001, faimtest_parse_genericerr, 0);
	aim_conn_addhandler(sess, bosconn, 0x0003, 0x0001, faimtest_parse_genericerr, 0);
	aim_conn_addhandler(sess, bosconn, 0x0009, 0x0001, faimtest_parse_genericerr, 0);
	aim_conn_addhandler(sess, bosconn, 0x0001, 0x000b, serverpause, 0);
	aim_conn_addhandler(sess, bosconn, 0x0001, 0x0012, migrate, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_ICQ, AIM_CB_ICQ_OFFLINEMSG, offlinemsg, 0);
	aim_conn_addhandler(sess, bosconn, AIM_CB_FAM_ICQ, AIM_CB_ICQ_OFFLINEMSGCOMPLETE, offlinemsgdone, 0);
	
#ifdef MID_REWROTE_ALL_THE_CRAP
	aim_conn_addhandler(sess, bosconn, 0xffff, 0xffff, faimtest_parse_unknown, 0);
#endif

	return;
}

