
#define FAIM_INTERNAL
#include <aim.h>

/* called for both reply and change-reply */
static int infochange(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
#ifdef MID_FINALLY_REWROTE_ALL_THE_CRAP
	int i;

	/*
	 * struct {
	 *    unsigned short perms;
	 *    unsigned short tlvcount;
	 *    aim_tlv_t tlvs[tlvcount];
	 *  } admin_info[n];
	 */
	for (i = 0; i < datalen; ) {
		int perms, tlvcount;

		perms = aimutil_get16(data+i);
		i += 2;

		tlvcount = aimutil_get16(data+i);
		i += 2;

		while (tlvcount) {
			aim_rxcallback_t userfunc;
			aim_tlv_t *tlv;
			int str = 0;

			if ((aimutil_get16(data+i) == 0x0011) ||
					(aimutil_get16(data+i) == 0x0004))
				str = 1;

			if (str)
				tlv = aim_grabtlvstr(data+i);
			else
				tlv = aim_grabtlv(data+i);

			/* XXX fix so its only called once for the entire packet */
			if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
				userfunc(sess, rx, perms, tlv->type, tlv->length, tlv->value, str);

			if (tlv)
				i += 2+2+tlv->length;

			if (tlv && tlv->value)
				free(tlv->value);
			if (tlv)
				free(tlv);

			tlvcount--;
		}
	}
#endif /* MID_FINALLY_REWROTE_ALL_THE_CRAP */
	return 1;
}

static int accountconfirm(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{
	aim_rxcallback_t userfunc;
	fu16_t status;

	status = aimbs_get16(bs);

	if ((userfunc = aim_callhandler(sess, rx->conn, snac->family, snac->subtype)))
		return userfunc(sess, rx, status);

	return 0;
}

static int snachandler(aim_session_t *sess, aim_module_t *mod, aim_frame_t *rx, aim_modsnac_t *snac, aim_bstream_t *bs)
{

	if ((snac->subtype == 0x0003) || (snac->subtype == 0x0005))
		return infochange(sess, mod, rx, snac, bs);
	else if (snac->subtype == 0x0007)
		return accountconfirm(sess, mod, rx, snac, bs);

	return 0;
}

faim_internal int admin_modfirst(aim_session_t *sess, aim_module_t *mod)
{

	mod->family = 0x0007;
	mod->version = 0x0000;
	mod->flags = 0;
	strncpy(mod->name, "admin", sizeof(mod->name));
	mod->snachandler = snachandler;

	return 0;
}

faim_export int aim_auth_clientready(aim_session_t *sess, aim_conn_t *conn)
{
	static const struct aim_tool_version tools[] = {
		{0x0001, 0x0003,    AIM_TOOL_NEWWIN, 0x0361},
		{0x0007, 0x0001,    AIM_TOOL_NEWWIN, 0x0361},
	};
	int j;
	aim_frame_t *tx;
	int toolcount = sizeof(tools) / sizeof(struct aim_tool_version);
	aim_snacid_t snacid;

	if (!(tx = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x0002, 1152)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0001, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&tx->data, 0x0001, 0x0002, 0x0000, snacid);

	for (j = 0; j < toolcount; j++) {
		aimbs_put16(&tx->data, tools[j].group);
		aimbs_put16(&tx->data, tools[j].version);
		aimbs_put16(&tx->data, tools[j].tool);
		aimbs_put16(&tx->data, tools[j].toolversion);
	}

	aim_tx_enqueue(sess, tx);

	return 0;
}

faim_export int aim_auth_changepasswd(aim_session_t *sess, aim_conn_t *conn, const char *newpw, const char *curpw)
{
	aim_frame_t *tx;
	aim_tlvlist_t *tl = NULL;
	aim_snacid_t snacid;

	if (!(tx = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+4+strlen(curpw)+4+strlen(newpw))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0007, 0x0004, 0x0000, NULL, 0);
	aim_putsnac(&tx->data, 0x0007, 0x0004, 0x0000, snacid);

	/* new password TLV t(0002) */
	aim_addtlvtochain_raw(&tl, 0x0002, strlen(newpw), newpw);

	/* current password TLV t(0012) */
	aim_addtlvtochain_raw(&tl, 0x0012, strlen(curpw), curpw);

	aim_writetlvchain(&tx->data, &tl);
	aim_freetlvchain(&tl);

	aim_tx_enqueue(sess, tx);

	return 0;
}

faim_export int aim_auth_setversions(aim_session_t *sess, aim_conn_t *conn)
{
	aim_frame_t *tx;
	aim_snacid_t snacid;

	if (!(tx = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 18)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0001, 0x0017, 0x0000, NULL, 0);
	aim_putsnac(&tx->data, 0x0001, 0x0017, 0x0000, snacid);

	aimbs_put16(&tx->data, 0x0001);
	aimbs_put16(&tx->data, 0x0003);

	aimbs_put16(&tx->data, 0x0007);
	aimbs_put16(&tx->data, 0x0001);

	aim_tx_enqueue(sess, tx);

	return 0;
}

/*
 * Request account confirmation. 
 *
 * This will cause an email to be sent to the address associated with
 * the account.  By following the instructions in the mail, you can
 * get the TRIAL flag removed from your account.
 *
 */
faim_export int aim_auth_reqconfirm(aim_session_t *sess, aim_conn_t *conn)
{
	return aim_genericreq_n(sess, conn, 0x0007, 0x0006);
}

/*
 * Request a bit of account info.
 *
 * The only known valid tag is 0x0011 (email address).
 *
 */ 
faim_export int aim_auth_getinfo(aim_session_t *sess, aim_conn_t *conn, fu16_t info)
{
	aim_frame_t *tx;
	aim_snacid_t snacid;

	if (!(tx = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 14)))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0002, 0x0002, 0x0000, NULL, 0);
	aim_putsnac(&tx->data, 0x0007, 0x0002, 0x0000, snacid);

	aimbs_put16(&tx->data, info);
	aimbs_put16(&tx->data, 0x0000);

	aim_tx_enqueue(sess, tx);

	return 0;
}

faim_export int aim_auth_setemail(aim_session_t *sess, aim_conn_t *conn, const char *newemail)
{
	aim_frame_t *tx;
	aim_snacid_t snacid;
	aim_tlvlist_t *tl = NULL;

	if (!(tx = aim_tx_new(sess, conn, AIM_FRAMETYPE_FLAP, 0x02, 10+2+2+strlen(newemail))))
		return -ENOMEM;

	snacid = aim_cachesnac(sess, 0x0007, 0x0004, 0x0000, NULL, 0);
	aim_putsnac(&tx->data, 0x0007, 0x0004, 0x0000, snacid);

	aim_addtlvtochain_raw(&tl, 0x0011, strlen(newemail), newemail);
	
	aim_writetlvchain(&tx->data, &tl);
	aim_freetlvchain(&tl);
	
	aim_tx_enqueue(sess, tx);

	return 0;
}
