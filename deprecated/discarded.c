/*
 * Hop in the Way-Back machine....  These are left over from the
 * decoding process back in Jun/Jul 1998 and are found to no longer
 * be necessary.  They're left here for reference _only_.  This file
 * should _not_ be linked into libfaim!
 */



/* 
   send_login_phase4_a(int socket)   

   Set ICBM Parameter?

*/
int aim_send_login_phase4_a_1(void)
{
  char command_1[] = {
    0x00, 0x04, 0x00, 0x02, 0x00, 0x00, 0x7a, 0x8c,
    0x11, 0x9c, 

    0x00, 0x01, 
    0x00, 0x00, 
    0x00, 0x03,
    0x1f, 0x3f, 
    0x03, 0xe7, 
    0x03, 0xe7, 
    0x00, 0x00, 
    0x00, 0x64
  };
  int command_1_len = 26;
  struct command_tx_struct newpacket;
  
  newpacket.lock = 1;
  newpacket.conn = aim_getconn_type(AIM_CONN_TYPE_BOS);
  newpacket.type = 0x02;
  newpacket.commandlen = command_1_len;
  newpacket.data = (char *) malloc (newpacket.commandlen);
  memcpy(newpacket.data, command_1, newpacket.commandlen);
  
  aim_tx_enqueue(&newpacket);

  return 0;
}
