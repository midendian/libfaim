
#define FAIM_INTERNAL
#include <aim.h>

/*
 * aim_add_buddy()
 *
 * Adds a single buddy to your buddy list after login.
 *
 * XXX this should just be an extension of setbuddylist()
 *
 */
faim_export unsigned long aim_add_buddy(struct aim_session_t *sess,
					struct aim_conn_t *conn, 
					char *sn )
{
   struct command_tx_struct *newpacket;
   int i;

   if(!sn)
     return -1;

   if (!(newpacket = aim_tx_new(AIM_FRAMETYPE_OSCAR, 0x0002, conn, 10+1+strlen(sn))))
     return -1;

   newpacket->lock = 1;

   i = aim_putsnac(newpacket->data, 0x0003, 0x0004, 0x0000, sess->snac_nextid);
   i += aimutil_put8(newpacket->data+i, strlen(sn));
   i += aimutil_putstr(newpacket->data+i, sn, strlen(sn));

   aim_tx_enqueue(sess, newpacket );

   aim_cachesnac(sess, 0x0003, 0x0004, 0x0000, sn, strlen(sn)+1);

   return sess->snac_nextid;
}

/*
 * XXX generalise to support removing multiple buddies (basically, its
 * the same as setbuddylist() but with a different snac subtype).
 *
 */
faim_export unsigned long aim_remove_buddy(struct aim_session_t *sess,
					   struct aim_conn_t *conn, 
					   char *sn )
{
   struct command_tx_struct *newpacket;
   int i;

   if(!sn)
     return -1;

   if (!(newpacket = aim_tx_new(AIM_FRAMETYPE_OSCAR, 0x0002, conn, 10+1+strlen(sn))))
     return -1;

   newpacket->lock = 1;

   i = aim_putsnac(newpacket->data, 0x0003, 0x0005, 0x0000, sess->snac_nextid);

   i += aimutil_put8(newpacket->data+i, strlen(sn));
   i += aimutil_putstr(newpacket->data+i, sn, strlen(sn));

   aim_tx_enqueue(sess, newpacket);

   aim_cachesnac(sess, 0x0003, 0x0005, 0x0000, sn, strlen(sn)+1);

   return sess->snac_nextid;
}

faim_internal int aim_parse_buddyrights(struct aim_session_t *sess,
					struct command_rx_struct *command, ...)
{
  rxcallback_t userfunc = NULL;
  int ret=1;
  struct aim_tlvlist_t *tlvlist;
  unsigned short maxbuddies = 0, maxwatchers = 0;

  /* 
   * TLVs follow 
   */
  if (!(tlvlist = aim_readtlvchain(command->data+10, command->commandlen-10)))
    return ret;

  /*
   * TLV type 0x0001: Maximum number of buddies.
   */
  if (aim_gettlv(tlvlist, 0x0001, 1))
    maxbuddies = aim_gettlv16(tlvlist, 0x0001, 1);

  /*
   * TLV type 0x0002: Maximum number of watchers.
   *
   * XXX: what the hell is a watcher? 
   *
   */
  if (aim_gettlv(tlvlist, 0x0002, 1))
    maxwatchers = aim_gettlv16(tlvlist, 0x0002, 1);
  
  if ((userfunc = aim_callhandler(command->conn, 0x0003, 0x0003)))
    ret =  userfunc(sess, command, maxbuddies, maxwatchers);

  aim_freetlvchain(&tlvlist);

  return ret;  
}