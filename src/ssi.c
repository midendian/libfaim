/*
 * Server-Side/Stored Information.
 *
 * Relatively new facility that allows storing of certain types of information,
 * such as a users buddy list, permit/deny list, and permit/deny preferences, 
 * to be stored on the server, so that they can be accessed from any client.
 *
 * This is entirely too complicated.
 *
 */

#define FAIM_INTERNAL
#include <aim.h>

/*
 * Request SSI Rights.
 */
faim_export int aim_ssi_reqrights(aim_session_t *sess, aim_conn_t *conn)
{
	return aim_genericreq_n(sess, conn, 0x0013, 0x0002);
}

/*
 * SSI Rights Information.
 */
static int parserights(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx);

	return ret;
}

/*
 * Request SSI Data.
 *
 * The data will only be sent if it is newer than the posted local
 * timestamp and revision.
 * 
 * Note that the client should never increment the revision, only the server.
 * 
 */
faim_export int aim_ssi_reqdata(aim_session_t *sess, aim_conn_t *conn, time_t localstamp, fu16_t localrev)
{
	aim_frame_t *fr;
	aim_snacid_t snacid;

	if (!sess || !conn)
		return -EINVAL;

	if (!(fr = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+4+2)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0013, 0x0005, 0x0000, NULL, 0);

	aim_putsnac(&fr->data, 0x0013, 0x0005, 0x0000, snacid);
	aimbs_put32(&fr->data, localstamp);
	aimbs_put16(&fr->data, localrev);

	aim_tx_enqueue(sess, fr);

	return 0;
}

/*
 * SSI Data.
 */
static int parsedata(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;
	struct aim_ssi_item *list = NULL;
	fu8_t fmtver;
	fu16_t rev;
	fu32_t stamp;

	fmtver = aimbs_get8(bs);
	rev = aimbs_get16(bs);

	while (aim_bstream_empty(bs) > 4) { /* last four bytes are stamp */
		fu16_t namelen, tbslen;
		struct aim_ssi_item *nl, *el;

		if (!(nl = malloc(sizeof(struct aim_ssi_item))))
			break;
		memset(nl, 0, sizeof(struct aim_ssi_item));

		if ((namelen = aimbs_get16(bs)))
			nl->name = aimbs_getstr(bs, namelen);
		nl->gid = aimbs_get16(bs);
		nl->bid = aimbs_get16(bs);
		nl->type = aimbs_get16(bs);

		if ((tbslen = aimbs_get16(bs))) {
			aim_bstream_t tbs;

			aim_bstream_init(&tbs, bs->data + bs->offset /* XXX */, tbslen);
			nl->data = (void *)aim_readtlvchain(&tbs);
			aim_bstream_advance(bs, tbslen);
		}

		for (el = list; el && el->next; el = el->next)
			;
		if (el)
			el->next = nl;
		else
			list = nl;
	}

	stamp = aimbs_get32(bs);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx, fmtver, rev, stamp, list);

	while (list) {
		struct aim_ssi_item *tmp;

		tmp = list->next;
		aim_freetlvchain((aim_tlvlist_t **)&list->data);
		free(list);
		list = tmp;
	}

	return ret;
}

/*
 * SSI Data Acknowledge.
 *
 * Should be sent after receiving 13/6 or 13/f.
 * 
 */
faim_export int aim_ssi_ackdata(aim_session_t *sess, aim_conn_t *conn)
{
	return aim_genericreq_n(sess, conn, 0x0013, 0x0007);
}

/*
 * SSI Data Unchanged.
 *
 * Response to aim_ssi_reqdata() if the server-side data is not newer than
 * posted local stamp/revision.
 *
 */
static int parsedataunchanged(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	int ret = 0;
	aim_rxcallback_t userfunc;

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		ret = userfunc(sess, rx);

	return ret;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if (snac->subtype == 0x0003)
		return parserights(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x006)
		return parsedata(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x00f)
		return parsedataunchanged(sess, mod, rx, snac, bs);

	return 0;
}

faim_internal int ssi_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x0013;
	mod->version = 0x0000;
	mod->flags = 0;
	strncpy(mod->name, "ssi", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}


