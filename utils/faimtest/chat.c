
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

	dvprintf("chat: %s:  New occupants have joined:\n", aim_chat_getname(fr->conn));
	for (i = 0; i < count; i++)
		dvprintf("chat: %s: \t%s\n", aim_chat_getname(fr->conn), userinfo[i].sn);

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

	dvprintf("chat: %s:  Some occupants have left:\n", aim_chat_getname(fr->conn));

	for (i = 0; i < count; i++)
		dvprintf("chat: %s: \t%s\n", aim_chat_getname(fr->conn), userinfo[i].sn);

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
	fu16_t flags, unknown_d2, unknown_d5, maxmsglen, maxvisiblemsglen;
	fu32_t creationtime;
	const char *croomname;

	croomname = aim_chat_getname(fr->conn);

	va_start(ap, fr);
	roominfo = va_arg(ap, struct aim_chat_roominfo *);
	roomname = va_arg(ap, char *);
	usercount = va_arg(ap, int);
	userinfo = va_arg(ap, aim_userinfo_t *);
	roomdesc = va_arg(ap, char *);
	flags = (fu16_t)va_arg(ap, unsigned int);
	creationtime = va_arg(ap, fu32_t);
	maxmsglen = (fu16_t)va_arg(ap, unsigned int);
	unknown_d2 = (fu16_t)va_arg(ap, unsigned int);
	unknown_d5 = (fu16_t)va_arg(ap, unsigned int);
	maxvisiblemsglen = (fu16_t)va_arg(ap, unsigned int);
	va_end(ap);

	dvprintf("chat: %s:  info update:\n", croomname);
	dvprintf("chat: %s:  \tRoominfo: {%04x, %s, %04x}\n", croomname, roominfo->exchange, roominfo->name, roominfo->instance);
	dvprintf("chat: %s:  \tRoomname: %s\n", croomname, roomname);
	dvprintf("chat: %s:  \tRoomdesc: %s\n", croomname, roomdesc);
	dvprintf("chat: %s:  \tOccupants: (%d)\n", croomname, usercount);

	for (i = 0; i < usercount; i++)
		dvprintf("chat: %s:  \t\t%s\n", croomname, userinfo[i].sn);

	dvprintf("chat: %s:  \tRoom flags: 0x%04x (%s%s%s%s)\n", 
			croomname, flags,
			(flags & AIM_CHATROOM_FLAG_EVILABLE) ? "Evilable, " : "",
			(flags & AIM_CHATROOM_FLAG_NAV_ONLY) ? "Nav Only, " : "",
			(flags & AIM_CHATROOM_FLAG_INSTANCING_ALLOWED) ? "Instancing allowed, " : "",
			(flags & AIM_CHATROOM_FLAG_OCCUPANT_PEEK_ALLOWED) ? "Occupant peek allowed, " : "");
	dvprintf("chat: %s:  \tCreation time: %lu (time_t)\n", croomname, creationtime);
	dvprintf("chat: %s:  \tUnknown_d2: 0x%04x\n", croomname, unknown_d2);
	dvprintf("chat: %s:  \tUnknown_d5: 0x%02x\n", croomname, unknown_d5);
	dvprintf("chat: %s:  \tMax message length: %d bytes\n", croomname, maxmsglen);
	dvprintf("chat: %s:  \tMax visible message length: %d bytes\n", croomname, maxvisiblemsglen);

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

	dvprintf("chat: %s: incoming msg from %s: %s\n", aim_chat_getname(fr->conn), userinfo->sn, msg);

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

		dprintf("chat info: Chat Rights:\n");
		dvprintf("chat info: \tMax Concurrent Rooms: %d\n", maxrooms);

		dvprintf("chat info: \tExchange List: (%d total)\n", exchangecount);
		for (i = 0; i < exchangecount; i++) {
			dvprintf("chat info: \t\t%x: %s (%s/%s) (0x%04x = %s%s%s%s)\n", 
			exchanges[i].number,
			exchanges[i].name,
			exchanges[i].charset1,
			exchanges[i].lang1,
			exchanges[i].flags,
			(exchanges[i].flags & AIM_CHATROOM_FLAG_EVILABLE) ? "Evilable, " : "",
			(exchanges[i].flags & AIM_CHATROOM_FLAG_NAV_ONLY) ? "Nav Only, " : "",
			(exchanges[i].flags & AIM_CHATROOM_FLAG_INSTANCING_ALLOWED) ? "Instancing allowed, " : "",
			(exchanges[i].flags & AIM_CHATROOM_FLAG_OCCUPANT_PEEK_ALLOWED) ? "Occupant peek allowed, " : "");
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

		dvprintf("received room create reply for %s/0x%04x\n", fqcn, exchange);

	} else {
		va_end(ap);
		dvprintf("chatnav info: unknown type (%04x)\n", type);
	}

	return 1;
}

static int conninitdone_chat(aim_session_t *sess, aim_frame_t *fr, ...)
{

	aim_clientready(sess, fr->conn);

	if (fr->conn->type == AIM_CONN_TYPE_CHATNAV) {

		dprintf("chatnav ready\n");
		
		aim_conn_addhandler(sess, fr->conn, 0x000d, 0x0001, faimtest_parse_genericerr, 0);
		aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CTN, AIM_CB_CTN_INFO, faimtest_chatnav_info, 0);

		aim_chatnav_reqrights(sess, fr->conn);
	
	} else if (fr->conn->type == AIM_CONN_TYPE_CHAT) {

		dprintf("chat ready\n");
		
		aim_conn_addhandler(sess, fr->conn, 0x000e, 0x0001, faimtest_parse_genericerr, 0);
		aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_USERJOIN, faimtest_chat_join, 0);
		aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_USERLEAVE, faimtest_chat_leave, 0);
		aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_ROOMINFOUPDATE, faimtest_chat_infoupdate, 0);
		aim_conn_addhandler(sess, fr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_INCOMINGMSG, faimtest_chat_incomingmsg, 0);

	}

	return 1;
}

void chatnav_redirect(aim_session_t *sess, struct aim_redirect_data *redir)
{
	aim_conn_t *tstconn;

	tstconn = aim_newconn(sess, AIM_CONN_TYPE_CHATNAV, redir->ip);
	if (!tstconn || (tstconn->status & AIM_CONN_STATUS_RESOLVERR)) {
		dprintf("unable to connect to chat(nav) server\n");
		if (tstconn)
			aim_conn_kill(sess, &tstconn);
		return;
	}

	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_chat, 0);

	aim_sendcookie(sess, tstconn, redir->cookie);

	dprintf("chatnav: connected\n");

	return;
}

/* XXX this needs instance too */
void chat_redirect(aim_session_t *sess, struct aim_redirect_data *redir)
{
	aim_conn_t *tstconn;

	tstconn = aim_newconn(sess, AIM_CONN_TYPE_CHAT, redir->ip);
	if (!tstconn || (tstconn->status & AIM_CONN_STATUS_RESOLVERR)) {
		dprintf("unable to connect to chat server\n");
		if (tstconn) 
			aim_conn_kill(sess, &tstconn);
		return; 
	}		
	dvprintf("chat: connected to %s instance %d on exchange %d\n", redir->chat.room, redir->chat.instance, redir->chat.exchange);

	/*
	 * We must do this to attach the stored name to the connection!
	 */
	aim_chat_attachname(tstconn, redir->chat.exchange, redir->chat.room, redir->chat.instance);

	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNCOMPLETE, faimtest_conncomplete, 0);
	aim_conn_addhandler(sess, tstconn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNINITDONE, conninitdone_chat, 0);

	aim_sendcookie(sess, tstconn, redir->cookie);

	return;	
}



