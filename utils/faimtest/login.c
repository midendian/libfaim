/*
 * Most of the stuff used for login.
 *
 */

#include "faimtest.h"

int logout(aim_session_t *sess)
{
	struct faimtest_priv *priv = (struct faimtest_priv *)sess->aux_data;

	if (priv->ohcaptainmycaptain)
		aim_send_im(sess, aim_getconn_type(sess, AIM_CONN_TYPE_BOS), priv->ohcaptainmycaptain, 0, "ta ta...");

	aim_session_kill(sess);

	if (faimtest_init() == -1)
		dprintf("faimtest_init failed\n");

	return 0;
}

static int faimtest_parse_login(aim_session_t *sess, aim_frame_t *fr, ...)
{
	struct faimtest_priv *priv = (struct faimtest_priv *)sess->aux_data;
	struct client_info_s info = AIM_CLIENTINFO_KNOWNGOOD;
	char *key;
	va_list ap;

	va_start(ap, fr);
	key = va_arg(ap, char *);
	va_end(ap);

	aim_send_login(sess, fr->conn, priv->screenname, priv->password, &info, key);

	return 1;
}

/* Does this thing even work anymore? */
static int faimtest_debugconn_connect(aim_session_t *sess, aim_frame_t *fr, ...)
{

	dprintf("faimtest: connecting to an aimdebugd!\n");

	/* convert the authorizer connection to a BOS connection */
	fr->conn->type = AIM_CONN_TYPE_BOS;

#if 0
	aim_conn_addhandler(sess, command->conn, AIM_CB_FAM_MSG, AIM_CB_MSG_INCOMING, faimtest_parse_incoming_im, 0);
#endif

	/* tell the aimddebugd we're ready */
	aim_debugconn_sendconnect(sess, fr->conn); 

	/* go right into main loop (don't open a BOS connection, etc) */
	return 1;
}


/*
 * This marks the end of authorization, and probably the beginning of BOS.
 *
 * If there was an error, everything ends here.  Otherwise, we use the cookie
 * and IP from the authorizer to connect to our assigned BOS.  This connection
 * will be used for the major stuff.  New connections may be opened later for
 * things like chat.
 *
 */
static int faimtest_parse_authresp(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_conn_t *bosconn;
	char *sn, *bosip, *errurl, *email;
	fu8_t *cookie;
	int errorcode, regstatus;
	int latestbuild, latestbetabuild;
	char *latestrelease, *latestbeta;
	char *latestreleaseurl, *latestbetaurl;
	char *latestreleaseinfo, *latestbetainfo;

	va_start(ap, fr);
	sn = va_arg(ap, char *);
	errorcode = va_arg(ap, int);
	errurl = va_arg(ap, char *);
	regstatus = va_arg(ap, int);
	email = va_arg(ap, char *);
	bosip = va_arg(ap, char *);
	cookie = va_arg(ap, fu8_t *);

	latestrelease = va_arg(ap, char *);
	latestbuild = va_arg(ap, int);
	latestreleaseurl = va_arg(ap, char *);
	latestreleaseinfo = va_arg(ap, char *);

	latestbeta = va_arg(ap, char *);
	latestbetabuild = va_arg(ap, int);
	latestbetaurl = va_arg(ap, char *);
	latestbetainfo = va_arg(ap, char *);

	va_end(ap);

	dvprintf("Screen name: %s\n", sn);

	/*
	 * Check for error.
	 */
	if (errorcode || !bosip || !cookie) {
		dvprintf("Login Error Code 0x%04x\n", errorcode);
		dvprintf("Error URL: %s\n", errurl);
		aim_conn_kill(sess, &fr->conn);
		return 1;
	}

	dvprintf("Reg status: %2d\n", regstatus);
	dvprintf("Email: %s\n", email);
	dvprintf("BOS IP: %s\n", bosip);

	if (latestbeta)
		dvprintf("Latest beta version: %s, build %d, at %s (more info at %s)\n", latestbeta, latestbetabuild, latestbetaurl, latestbetainfo);

	if (latestrelease)
		dvprintf("Latest released version: %s, build %d, at %s (more info at %s)\n", latestrelease, latestbuild, latestreleaseurl, latestreleaseinfo);

	dprintf("Closing auth connection...\n");
	aim_conn_kill(sess, &fr->conn);

	if (!(bosconn = aim_newconn(sess, AIM_CONN_TYPE_BOS, bosip))) {
		dprintf("could not connect to BOS: internal error\n");
		return 1;
	} else if (bosconn->status & AIM_CONN_STATUS_CONNERR) {	
		dprintf("could not connect to BOS\n");
		aim_conn_kill(sess, &bosconn);
		return 1;
	}

	addcb_bos(sess, bosconn);

	aim_auth_sendcookie(sess, bosconn, cookie);

	return 1;
}

int login(aim_session_t *sess, const char *sn, const char *passwd)
{
	struct faimtest_priv *priv = (struct faimtest_priv *)sess->aux_data;
	aim_conn_t *authconn;

	if (sn)
		priv->screenname = strdup(sn);
	if (passwd)
		priv->password = strdup(passwd);

	if (priv->proxy)
		aim_setupproxy(sess, priv->proxy, priv->proxyusername, priv->proxypass);

	if (!priv->screenname || !priv->password) {
		dprintf("need SN and password\n");
		return -1;
	}

	if (!(authconn = aim_newconn(&aimsess, AIM_CONN_TYPE_AUTH, 
			priv->server ? priv->server : FAIM_LOGIN_SERVER))) {
		dprintf("internal connection error during login\n");
		return -1;
	} else if (authconn->fd == -1) {

		if (authconn->status & AIM_CONN_STATUS_RESOLVERR) {
			dprintf("could not resolve authorizer name\n");
		} else if (authconn->status & AIM_CONN_STATUS_CONNERR) {
			dprintf("could not connect to authorizer\n");
		}
		aim_conn_kill(sess, &authconn);

		return -1;
	}

	aim_conn_addhandler(sess, authconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_FLAPVER, faimtest_flapversion, 0);
	aim_conn_addhandler(sess, authconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
	aim_conn_addhandler(sess, authconn, 0x0017, 0x0007, faimtest_parse_login, 0);
	aim_conn_addhandler(sess, authconn, 0x0017, 0x0003, faimtest_parse_authresp, 0);    

	aim_conn_addhandler(sess, authconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_DEBUGCONN_CONNECT, faimtest_debugconn_connect, 0);

	/* If the connection is in progress, this will just be queued */
	aim_request_login(sess, authconn, priv->screenname);

	dprintf("login request sent\n");

	return 0;
}


