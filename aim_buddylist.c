
#include <faim/aim.h>

/*
 * aim_add_buddy()
 *
 * Adds a single buddy to your buddy list after login.
 *
 */
u_long aim_add_buddy(struct aim_session_t *sess,
		     struct aim_conn_t *conn, 
		     char *sn )
{
   struct command_tx_struct newpacket;

   if( !sn )
      return -1;

   if (conn)
     newpacket.conn = conn;
   else
     newpacket.conn = aim_getconn_type(sess, AIM_CONN_TYPE_BOS);

   newpacket.lock = 1;
   newpacket.type = 0x0002;
   newpacket.commandlen = 11 + strlen( sn );
   newpacket.data = (char *)malloc( newpacket.commandlen );

   aim_putsnac(newpacket.data, 0x0003, 0x0004, 0x0000, sess->snac_nextid);

   /* length of screenname */ 
   newpacket.data[10] = strlen( sn );

   memcpy( &(newpacket.data[11]), sn, strlen( sn ) );

   aim_tx_enqueue(sess, &newpacket );

   {
      struct aim_snac_t snac;
    
      snac.id = sess->snac_nextid;
      snac.family = 0x0003;
      snac.type = 0x0004;
      snac.flags = 0x0000;

      snac.data = malloc( strlen( sn ) + 1 );
      memcpy( snac.data, sn, strlen( sn ) + 1 );

      aim_newsnac(sess, &snac);
   }

   return( sess->snac_nextid++ );
}

u_long aim_remove_buddy(struct aim_session_t *sess,
			struct aim_conn_t *conn, 
			char *sn )
{
   struct command_tx_struct newpacket;

   if( !sn )
      return -1;

   if (conn)
     newpacket.conn = conn;
   else
     newpacket.conn = aim_getconn_type(sess, AIM_CONN_TYPE_BOS);

   newpacket.lock = 1;
   newpacket.type = 0x0002;
   newpacket.commandlen = 11 + strlen(sn);
   newpacket.data = (char *)malloc( newpacket.commandlen );

   aim_putsnac(newpacket.data, 0x0003, 0x0005, 0x0000, sess->snac_nextid);

   /* length of screenname */ 
   newpacket.data[10] = strlen( sn );

   memcpy( &(newpacket.data[11]), sn, strlen( sn ) );

   aim_tx_enqueue(sess,  &newpacket );

   {
      struct aim_snac_t snac;
    
      snac.id = sess->snac_nextid;
      snac.family = 0x0003;
      snac.type = 0x0005;
      snac.flags = 0x0000;

      snac.data = malloc( strlen( sn ) + 1 );
      memcpy( snac.data, sn, strlen( sn ) + 1 );

      aim_newsnac(sess, &snac );
   }

   return( sess->snac_nextid++ );
}

