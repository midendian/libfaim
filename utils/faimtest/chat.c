
#include "faimtest.h"

static int faimtest_chat_join(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_userinfo_t *userinfo;
	int count, i;

	va_start(ap, fr);
	count = va_arg(ap, int);
	userinfo = va_arg(ap, aim_userinfo_t *);
	va_end(ap);

	dvprintf("faimtest: chat: %s:  New occupants have joined:\n", (char *)fr->conn->priv);
	for (i = 0; i < count; i++)
		dvprintf("faimtest: chat: %s: \t%s\n", (char *)fr->conn->priv, userinfo[i].sn);

	return 1;
}

static int faimtest_chat_leave(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_userinfo_t *userinfo;
	int count , i;

	va_start(ap, fr);
	count = va_arg(ap, int);
	userinfo = va_arg(ap, aim_userinfo_t *);
	va_end(ap);

	dvprintf("faimtest: chat: %s:  Some occupants have left:\n", (char *)fr->conn->priv);

	for (i = 0; i < count; i++)
		dvprintf("faimtest: chat: %s: \t%s\n", (char *)fr->conn->priv, userinfo[i].sn);

	return 1;
}

static int faimtest_chat_infoupdate(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_userinfo_t *userinfo;
	struct aim_chat_roominfo *roominfo;
	char *roomname;
	int usercount, i;
	char *roomdesc;
	fu16_t unknown_c9, unknown_d2, unknown_d5, maxmsglen, maxvisiblemsglen;
	fu32_t creationtime;
	const char *croomname = (const char *)fr->conn->priv;

	va_start(ap, fr);
	roominfo = va_arg(ap, struct aim_chat_roominfo *);
	roomname = va_arg(ap, char *);
	usercount = va_arg(ap, int);
	userinfo = va_arg(ap, aim_userinfo_t *);
	roomdesc = va_arg(ap, char *);
	unknown_c9 = (fu16_t)va_arg(ap, unsigned int);
	creationtime = va_arg(ap, fu32_t);
	maxmsglen = (fu16_t)va_arg(ap, unsigned int);
	unknown_d2 = (fu16_t)va_arg(ap, unsigned int);
	unknown_d5 = (fu16_t)va_arg(ap, unsigned int);
	maxvisiblemsglen = (fu16_t)va_arg(ap, unsigned int);
	va_end(ap);

	dvprintf("faimtest: chat: %s:  info update:\n", croomname);
	dvprintf("faimtest: chat: %s:  \tRoominfo: {%04x, %s, %04x}\n", croomname, roominfo->exchange, roominfo->name, roominfo->instance);
	dvprintf("faimtest: chat: %s:  \tRoomname: %s\n", croomname, roomname);
	dvprintf("faimtest: chat: %s:  \tRoomdesc: %s\n", croomname, roomdesc);
	dvprintf("faimtest: chat: %s:  \tOccupants: (%d)\n", croomname, usercount);

	for (i = 0; i < usercount; i++)
		dvprintf("faimtest: chat: %s:  \t\t%s\n", croomname, userinfo[i].sn);

	dvprintf("faimtest: chat: %s:  \tUnknown_c9: 0x%04x\n", croomname, unknown_c9);
	dvprintf("faimtest: chat: %s:  \tCreation time: %lu (time_t)\n", croomname, creationtime);
	dvprintf("faimtest: chat: %s:  \tUnknown_d2: 0x%04x\n", croomname, unknown_d2);
	dvprintf("faimtest: chat: %s:  \tUnknown_d5: 0x%02x\n", croomname, unknown_d5);
	dvprintf("faimtest: chat: %s:  \tMax message length: %d bytes\n", croomname, maxmsglen);
	dvprintf("faimtest: chat: %s:  \tMax visible message length: %d bytes\n", croomname, maxvisiblemsglen);

	return 1;
}

static int faimtest_chat_incomingmsg(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_userinfo_t *userinfo;
	char *msg;
	char tmpbuf[1152];

	va_start(ap, fr);
	userinfo = va_arg(ap, aim_userinfo_t *);	
	msg = va_arg(ap, char *);
	va_end(ap);

	dvprintf("faimtest: chat: %s: incoming msg from %s: %s\n", (char *)fr->conn->priv, userinfo->sn, msg);

	/*
	 * Do an echo for testing purposes.  But not for ourselves ("oops!")
	 */
	if (strcmp(userinfo->sn, sess->sn) != 0) {
		sprintf(tmpbuf, "(%s said \"%s\")", userinfo->sn, msg);
		aim_chat_send_im(sess, fr->conn, 0, tmpbuf, strlen(tmpbuf));
	}

	return 1;
}

static int faimtest_chatnav_info(aim_session_t *sess, aim_frame_t *fr, ...)
{
	fu16_t type;
	va_list ap;

	va_start(ap, fr);
	type = (fu16_t)va_arg(ap, unsigned int);

	if (type == 0x0002) {
		int maxrooms;
		struct aim_chat_exchangeinfo *exchanges;
		int exchangecount, i;

		maxrooms = va_arg(ap, int);
		exchangecount = va_arg(ap, int);
		exchanges = va_arg(ap, struct aim_chat_exchangeinfo *);
		va_end(ap);

		dprintf("faimtest: chat info: Chat Rights:\n");
		dvprintf("faimtest: chat info: \tMax Concurrent Rooms: %d\n", maxrooms);

		dvprintf("faimtest: chat info: \tExchange List: (%d total)\n", exchangecount);
		for (i = 0; i < exchangecount; i++) {
			dvprintf("faimtest: chat info: \t\t%x: %s (%s/%s)\n", 
			exchanges[i].number,	
			exchanges[i].name,
			exchanges[i].charset1,
			exchanges[i].lang1);
		}

	} else if (type == 0x0008) {
		char *fqcn, *name, *ck;
		fu16_t instance, flags, maxmsglen, maxoccupancy, unknown, exchange;
		fu8_t createperms;
		fu32_t createtime;

		fqcn = va_arg(ap, char *);
		instance = (fu16_t)va_arg(ap, unsigned int);
		exchange = (fu16_t)va_arg(ap, unsigned int);
		flags = (fu16_t)va_arg(ap, unsigned int);
		createtime = va_arg(ap, fu32_t);
		maxmsglen = (fu16_t)va_arg(ap, unsigned int);
		maxoccupancy = (fu16_t)va_arg(ap, unsigned int);
		createperms = (fu8_t)va_arg(ap, unsigned int);
		unknown = (fu16_t)va_arg(ap, unsigned int);
		name = va_arg(ap, char *);
		ck = va_arg(ap, char *);
		va_end(ap);

		dvprintf("faimtest: received room create reply for %s/0x%04x\n", fqcn, exchange);

	} else {
		va_end(ap);
		dvprintf("faimtest: chatnav info: unknown type (%04x)\n", type);
	}

	return 1;
}

static int conninitdone_chat(aim_session_t *sess, aim_frame_t *fr, ...)
{

	aim_clientready(sess, fr->conn);

	if (fr->conn->type == AIM_CONN_TYPE_CHATNAV) {

		dprintf("chatnav ready\n");
		
		aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CTN, AIM_CB_CTN_INFO, faimtest_chatnav_info, 0);

		aim_chatnav_reqrights(sess, fr->conn);
	
	} else if (fr->conn->type == AIM_CONN_TYPE_CHAT) {

		dprintf("chat ready\n");
		
		aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_USERJOIN, faimtest_chat_join, 0);
		aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_USERLEAVE, faimtest_chat_leave, 0);
		aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_ROOMINFOUPDATE, faimtest_chat_infoupdate, 0);
		aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_INCOMINGMSG, faimtest_chat_incomingmsg, 0);

	}

	return 1;
}

void chatnav_redirect(aim_session_t *sess, const char *ip, const fu8_t *cookie)
{
	aim_conn_t *tstconn;

	tstconn = aim_newconn(sess, AIM_CONN_TYPE_CHATNAV, ip);
	if (!tstconn || (tstconn->status & AIM_CONN_STATUS_RESOLVERR)) {
		dprintf("faimtest: unable to connect to chat(nav) server\n");
		if (tstconn)
			aim_conn_kill(sess, &tstconn);
		return;
	}

	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_chat, 0);

	aim_auth_sendcookie(sess, tstconn, cookie);

	dprintf("chatnav: connected\n");

	return;
}

/* XXX this needs instance too */
void chat_redirect(aim_session_t *sess, const char *ip, const fu8_t *cookie, const char *roomname, fu16_t exchange)
{
	aim_conn_t *tstconn;

	tstconn = aim_newconn(sess, AIM_CONN_TYPE_CHAT, ip);
	if (!tstconn || (tstconn->status & AIM_CONN_STATUS_RESOLVERR)) {
		dprintf("faimtest: unable to connect to chat server\n");
		if (tstconn) 
			aim_conn_kill(sess, &tstconn);
		return; 
	}		
	dvprintf("faimtest: chat: connected to %s on exchange %d\n", roomname, exchange);

	/*
	 * We must do this to attach the stored name to the connection!
	 */
	aim_chat_attachname(tstconn, roomname);

	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_chat, 0);

	aim_auth_sendcookie(sess, tstconn, cookie);

	return;	
}



