
#include <faim/aim.h>

int sendimtoclient(struct aim_session_t *sess, struct aim_conn_t *conn,
		   char *srcsn, u_int flags, char *msg)
{
  struct command_tx_struct *tx;
  struct aimd_clientinfo *client = NULL;
  struct aim_tlvlist_t *tlvlist = NULL;
  struct aim_tlvlist_t *tlvlist2 = NULL;
  int i = 0, y, z;
  char *msgblock;

  if (!(tx = aim_tx_new(AIM_FRAMETYPE_OSCAR, 0x0002, conn, strlen(msg)+256)))
    return -1;

  tx->lock = 1;

  msgblock = malloc(strlen(msg)+128); /* ?? */
  memset(msgblock, 0, strlen(msg)+128);

  z = 0;
  z += aimutil_put8(msgblock+z, 0x00);
  z += aimutil_put8(msgblock+z, 0x00);

  z += aimutil_put16(msgblock+z, 0x02);
  z += aimutil_put8(msgblock+z, 0x00);
  z += aimutil_put8(msgblock+z, 0x00);

  z += aimutil_put8(msgblock+z, 0x00);
  z += aimutil_put8(msgblock+z, 0x00);

  z += aimutil_put16(msgblock+z, strlen(msg)+4);
  /* flag words */
  z += aimutil_put16(msgblock+z, 0x0000);
  z += aimutil_put16(msgblock+z, 0x0000);

  /* msg */
  z += aimutil_putstr(msgblock+z, msg, strlen(msg));

  /*
   * SNAC header
   */
  i = 0;
  i += aimutil_put16(tx->data+i, 0x0004);
  i += aimutil_put16(tx->data+i, 0x0007);
  i += aimutil_put16(tx->data+i, 0x0000);
  i += aimutil_put16(tx->data+i, 0x0000);
  i += aimutil_put16(tx->data+i, 0x0000);

  /*
   * Message cookie
   */
  for (y=0;y<8;y++)
    i += aimutil_put8(tx->data+i+y, (u_char) random());

  /*
   * Channel ID
   */
  i += aimutil_put16(tx->data+i, 0x0001);

  /*
   * Source SN
   */
  i += aimutil_put8(tx->data+i, strlen(srcsn));
  i += aimutil_putstr(tx->data+i, srcsn, strlen(srcsn));

  /*
   * Warning level
   */
  i += aimutil_put16(tx->data+i, 0x0000);

  /* class */
  aim_addtlvtochain16(&tlvlist, 0x0001, AIM_FLAG_FREE | AIM_FLAG_UNCONFIRMED);

  /* member-since date */
  aim_addtlvtochain32(&tlvlist, 0x0002, 0);

  /* on-since date */
  aim_addtlvtochain32(&tlvlist, 0x0003, 0);

  /* idle-time */
  aim_addtlvtochain16(&tlvlist, 0x0004, 0);

  /* session length (AIM) */
  aim_addtlvtochain16(&tlvlist, 0x000f, 0);

  /* add msgblock to chain */
  aim_addtlvtochain_str(&tlvlist2, 0x0002, msgblock, z);

  i += aimutil_put16(tx->data+i, aim_counttlvchain(&tlvlist));
  i += aim_writetlvchain(tx->data+i, tx->commandlen-i, &tlvlist);
  tx->commandlen = aim_writetlvchain(tx->data+i, tx->commandlen-i, &tlvlist2)+i;
  

  free(msgblock);
  aim_freetlvchain(&tlvlist);

  tx->lock = 0;

  aim_tx_enqueue(sess, tx);

  sess->snac_nextid++;

  return 0;
}
