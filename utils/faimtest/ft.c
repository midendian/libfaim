
#include "faimtest.h"

static int directim_incoming(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	char *sn, *msg;

	va_start(ap, fr);
	sn = va_arg(ap, char *);
	msg = va_arg(ap, char *);
	va_end(ap);

	dvprintf("Directim from %s: %s\n", sn, msg);

	if (strstr(msg, "sendmsg")) {
		int i;

		i = atoi(strstr(msg, "sendmsg")+8);
		if (i < 10000) {
			char *newbuf;
			int z;

			newbuf = malloc(i+1);
			for (z = 0; z < i; z++)
				newbuf[z] = (z % 10)+0x30;
			newbuf[i] = '\0';
			aim_send_im_direct(sess, fr->conn, newbuf);
			free(newbuf);
		}

	} else if (strstr(msg, "goodday")) {

		aim_send_im_direct(sess, fr->conn, "Good day to you, too");

	} else {
		char newmsg[1024];

		snprintf(newmsg, sizeof(newmsg), "unknown (%s)\n", msg);
		aim_send_im_direct(sess, fr->conn, newmsg);
	}

	return 1;
}

static int directim_typing(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	char *sn;

	va_start(ap, fr);
	sn = va_arg(ap, char *);
	va_end(ap);

	dvprintf("ohmigod! %s has started typing (DirectIM). He's going to send you a message! *squeal*\n", sn);

	return 1;
}

static int faimtest_directim_initiate(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_conn_t *newconn, *listenerconn;

	va_start(ap, fr);
	newconn = va_arg(ap, aim_conn_t *);
	listenerconn = va_arg(ap, aim_conn_t *);
	va_end(ap);

	aim_conn_close(listenerconn);
	aim_conn_kill(sess, &listenerconn);

	aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMINCOMING, directim_incoming, 0);
	aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMTYPING, directim_typing, 0);

	aim_send_im_direct(sess, newconn, "goodday");

	dvprintf("OFT: DirectIM: connected to %s\n", aim_directim_getsn(newconn));

	return 1;
}

static int faimtest_getfile_filereq(aim_session_t *ses, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_conn_t *oftconn;
	struct aim_fileheader_t *fh;
	fu8_t *cookie;

	va_start(ap, fr);
	oftconn = va_arg(ap, aim_conn_t *);
	fh = va_arg(ap, struct aim_fileheader_t *);
	cookie = va_arg(ap, fu8_t *);
	va_end(ap);

	dvprintf("request for file %s.\n", fh->name);

	return 1;
}

static int faimtest_getfile_filesend(aim_session_t *sess, aim_frame_t *fr, ...)
{
	struct faimtest_priv *priv = (struct faimtest_priv *)sess->aux_data;
	va_list ap;
	aim_conn_t *oftconn;
	struct aim_fileheader_t *fh;
	char *path;
	fu8_t *cookie;
	int pos, bufpos = 0, bufsize = 2048, i;
	char *buf;
	FILE *file;

	va_start(ap, fr);
	oftconn = va_arg(ap, aim_conn_t *);
	fh = va_arg(ap, struct aim_fileheader_t *);
	cookie = va_arg(ap, fu8_t *);
	va_end(ap);

	dvprintf("sending file %s(%ld).\n", fh->name, fh->size);

	if (!(buf = malloc(2048)))
		return -1;

	if (!(path = (char *)calloc(1, strlen(priv->listingpath) +strlen(fh->name)+2))) {
		dperror("calloc");
		dprintf("error in calloc of path\n");

		return 0; /* XXX: no idea what winaim expects here =) */
	}

	snprintf(path, strlen(priv->listingpath)+strlen(fh->name)+2, "%s/%s", priv->listingpath, fh->name);

	if (!(file = fopen(path, "r"))) {
		dvprintf("getfile_send fopen failed for %s. damn.\n", path);
		return 0;
	}

	/* 
	 * This is a mess. Remember that faimtest is demonstration code
	 * only and for the sake of the gods, don't use this code in any
	 * of your clients. --mid
	 */
	for(pos = 0; pos < fh->size; pos++) {

		bufpos = pos % bufsize;

		if (bufpos == 0 && pos > 0) { /* filled our buffer. spit it across the wire */
			if ( (i = send(oftconn->fd, buf, bufsize, 0)) != bufsize ) {
				dperror("faim: getfile_send: write1");
				dprintf("faim: getfile_send: whoopsy, didn't write it all...\n");
				free(buf);   
				return 0;
			}
		}

		if( (buf[bufpos] = fgetc(file)) == EOF) {
			if(pos != fh->size) {
				dvprintf("faim: getfile_send: hrm... apparent early EOF at pos 0x%x of 0x%lx\n", pos, fh->size);
				free(buf);   
				return 0;
			}
		}      
		dvprintf("%c(0x%02x) ", buf[pos], buf[pos]);
	}

	if( (i = send(oftconn->fd, buf, bufpos+1, 0)) != (bufpos+1)) {
		dperror("faim: getfile_send: write2");
		dprintf("faim: getfile_send cleanup: whoopsy, didn't write it all...\n");
		free(buf);   
		return 0;
	}

	free(buf);
	free(fh);

	return 1;
}

static int faimtest_getfile_complete(aim_session_t *sess, aim_frame_t *fr, ...) 
{
	va_list ap;
	aim_conn_t *conn;
	struct aim_fileheader_t *fh;

	va_start(ap, fr);
	conn = va_arg(ap, aim_conn_t *);
	fh = va_arg(ap, struct aim_fileheader_t *);
	va_end(ap);

	dvprintf("completed file transfer for %s.\n", fh->name);

	aim_conn_close(conn);
	aim_conn_kill(sess, &conn);

	return 1;
}

static int faimtest_getfile_disconnect(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_conn_t *conn;
	char *sn;

	va_start(ap, fr);
	conn = va_arg(ap, aim_conn_t *);
	sn = va_arg(ap, char *);
	va_end(ap);

	aim_conn_kill(sess, &conn);

	dvprintf("getfile: disconnected from %s\n", sn);

	return 1;
}

static int faimtest_getfile_listing(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_conn_t *conn;
	char *listing;
	struct aim_filetransfer_priv *ft;
	char *filename, *nameend, *sizec;
	int filesize, namelen;

	va_start(ap, fr);
	conn = va_arg(ap, aim_conn_t *);
	ft = va_arg(ap, struct aim_filetransfer_priv *);
	listing = va_arg(ap, char *);
	va_end(ap);

	dvprintf("listing on %d==================\n%s\n===========\n", conn->fd, listing);

	nameend = strstr(listing+0x1a, "\r");

	namelen = nameend - (listing + 0x1a);

	filename = malloc(namelen + 1);
	strncpy(filename, listing+0x1a, namelen);
	filename[namelen] = 0x00;

	sizec = malloc(8+1);
	memcpy(sizec, listing + 0x11, 8);
	sizec[8] = 0x00;

	filesize =  strtol(sizec, (char **)NULL, 10);

	dvprintf("requesting %d %s(%d long)\n", namelen, filename, filesize);

	aim_oft_getfile_request(sess, conn, filename, filesize);

	free(filename);
	free(sizec);

	return 0;
}

static int faimtest_getfile_listingreq(aim_session_t *sess, aim_frame_t *fr, ...)
{
	struct faimtest_priv *priv = (struct faimtest_priv *)sess->aux_data;
	va_list ap;
	aim_conn_t *oftconn;
	struct aim_fileheader_t *fh;
	int pos, bufpos = 0, bufsize = 2048, i;
	char *buf;

	va_start(ap, fr);
	oftconn = va_arg(ap, aim_conn_t *);
	fh = va_arg(ap, struct aim_fileheader_t *);
	va_end(ap);

	dvprintf("sending listing of size %ld\n", fh->size);

	if(!(buf = malloc(2048)))
		return -1;

	for(pos = 0; pos < fh->size; pos++) {

		bufpos = pos % bufsize;

		if(bufpos == 0 && pos > 0) { /* filled our buffer. spit it across the wire */
			if ( (i = send(oftconn->fd, buf, bufsize, 0)) != bufsize ) {
				dperror("faim: getfile_send: write1");
				dprintf("faim: getfile_send: whoopsy, didn't write it all...\n");
				free(buf);   
				return 0;
			}
		}
		if( (buf[bufpos] = fgetc(priv->listingfile)) == EOF) {
			if(pos != fh->size) {
				dvprintf("faim: getfile_send: hrm... apparent early EOF at pos 0x%x of 0x%lx\n", pos, fh->size);
				free(buf);   
				return 0;
			}
		}      
	}

	if( (i = send(oftconn->fd, buf, bufpos+1, 0)) != (bufpos+1)) {
		dperror("faim: getfile_send: write2");
		dprintf("faim: getfile_send cleanup: whoopsy, didn't write it all...\n");
		free(buf);   
		return 0;
	}

	dprintf("sent listing\n");
	free(buf);

	return 0;
}

static int faimtest_getfile_receive(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_conn_t *conn;
	struct aim_filetransfer_priv *ft;
	unsigned char data;
	int pos;

	va_start(ap, fr);
	conn = va_arg(ap, aim_conn_t *);
	ft = va_arg(ap, struct aim_filetransfer_priv *);
	va_end(ap);

	dvprintf("receiving %ld bytes of file data for %s:\n\t", ft->fh.size, ft->fh.name);

	for(pos = 0; pos < ft->fh.size; pos++) {
		read(conn->fd, &data, 1);
		printf("%c(%02x) ", data, data);
	}

	printf("\n");

	aim_oft_getfile_end(sess, conn);

	return 0;
}

static int faimtest_getfile_state4(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_conn_t *conn;

	va_start(ap, fr);
	conn = va_arg(ap, aim_conn_t *);
	va_end(ap);

	aim_conn_close(conn);
	aim_conn_kill(sess, &conn);

	return 0;
}

static int faimtest_getfile_initiate(aim_session_t *sess, aim_frame_t *fr, ...)
{
	va_list ap;
	aim_conn_t *conn, *listenerconn;
	struct aim_filetransfer_priv *priv;

	va_start(ap, fr);
	conn = va_arg(ap, aim_conn_t *);
	listenerconn = va_arg(ap, aim_conn_t *);
	va_end(ap);

	aim_conn_close(listenerconn);
	aim_conn_kill(sess, &listenerconn);

	aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEFILEREQ,  faimtest_getfile_filereq, 0);
	aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEFILESEND, faimtest_getfile_filesend, 0);
	aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILECOMPLETE, faimtest_getfile_complete, 0);      
	aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEDISCONNECT, faimtest_getfile_disconnect, 0);      
	aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILELISTING, faimtest_getfile_listing, 0);
	aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILELISTINGREQ, faimtest_getfile_listingreq, 0);
	aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILERECEIVE, faimtest_getfile_receive, 0);
	aim_conn_addhandler(sess, conn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILESTATE4, faimtest_getfile_state4, 0);

	priv = (struct aim_filetransfer_priv *)conn->priv;

	dvprintf("getfile: %s (%s) connected to us on %d\n", priv->sn, priv->ip, conn->fd);

	return 1;
}

void getfile_start(aim_session_t *sess, aim_conn_t *conn, const char *sn)
{
	aim_conn_t *newconn;

	newconn = aim_getfile_initiate(sess, conn, sn);
	dvprintf("getting file listing from %s\n", sn);
	aim_conn_addhandler(sess, newconn,  AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEINITIATE, faimtest_getfile_initiate,0);

	return;
}

void getfile_requested(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args)
{
	struct faimtest_priv *priv = (struct faimtest_priv *)sess->aux_data;
	aim_conn_t *newconn;
	struct aim_fileheader_t *fh;

	dvprintf("get file request from %s (at %s/%s/%s) %x\n", userinfo->sn, args->clientip, args->clientip2, args->verifiedip, args->reqclass);

	fh = aim_getlisting(sess, priv->listingfile);

	newconn = aim_accepttransfer(sess, conn, userinfo->sn, args->cookie, args->clientip, fh->totfiles, fh->totsize, fh->size, fh->checksum, args->reqclass);

	free(fh);

	if ( (!newconn) || (newconn->fd == -1) ) {

		dprintf("getfile: requestconn: apparent error in accepttransfer\n");

		if (newconn)
			aim_conn_kill(sess, &newconn);

		return;
	}


	aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILELISTINGREQ, faimtest_getfile_listingreq, 0);
	aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEFILEREQ,  faimtest_getfile_filereq, 0);
	aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEFILESEND, faimtest_getfile_filesend, 0);
	aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILECOMPLETE, faimtest_getfile_complete, 0);      

	aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_GETFILEDISCONNECT, faimtest_getfile_disconnect, 0);      

	dprintf("getfile connect succeeded, handlers added.\n");

	return;
}

void directim_start(aim_session_t *sess, const char *sn)
{
	aim_conn_t *newconn;

	printf("opening directim to %s\n", sn);
 	
	newconn = aim_directim_initiate(sess, sn);

	if (!newconn || (newconn->fd == -1)) {

		printf("connection failed!\n");

		if (newconn)
			aim_conn_kill(sess, &newconn);

	} else
		aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMINITIATE, faimtest_directim_initiate, 0);

	return;
}

void directim_requested(aim_session_t *sess, aim_conn_t *conn, aim_userinfo_t *userinfo, struct aim_incomingim_ch2_args *args)
{
	aim_conn_t *newconn;

	dvprintf("OFT: DirectIM: request from %s (%s/%s/%s)\n", userinfo->sn, args->clientip, args->clientip2, args->verifiedip);

	newconn = aim_directim_connect(sess, userinfo->sn, args->clientip, args->cookie);

	if (!newconn || (newconn->fd == -1)) {

		dprintf("icbm: imimage: could not connect\n");

		if (newconn)
			aim_conn_kill(sess, &newconn);

	} else {

		aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMINCOMING, directim_incoming, 0);
		aim_conn_addhandler(sess, newconn, AIM_CB_FAM_OFT, AIM_CB_OFT_DIRECTIMTYPING, directim_typing, 0);

		dvprintf("OFT: DirectIM: connected to %s\n", userinfo->sn);

		aim_send_im_direct(sess, newconn, "goodday");
	}

}

