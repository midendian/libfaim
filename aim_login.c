/*
 *  aim_login.c
 *
 *  This contains all the functions needed to actually login.
 *
 */

#include "aim.h"


/*
 * FIXME: Reimplement the TIS stuff.
 */
#ifdef TIS_TELNET_PROXY
#include "tis_telnet_proxy.h"
#endif

/*
 *  send_login(int socket, char *sn, char *password)
 *  
 * This is the initial login request packet.
 *
 * The password is encoded before transmition, as per
 * encode_password().  See that function for their
 * stupid method of doing it.
 *
 *
 *
 */
int aim_send_login (struct aim_conn_t *conn, char *sn, char *password, struct client_info_s *clientinfo)
#if 0
{
  char *password_encoded = NULL;  /* to store encoded password */
  int curbyte=0;

  struct command_tx_struct newpacket;

  if (conn)
    newpacket.conn = conn;
  else
    newpacket.conn = aim_getconn_type(AIM_CONN_TYPE_AUTH);

  newpacket.commandlen = 6+2+strlen(sn)+1+1+2+strlen(password)+6;

  if (clientinfo)
    {
      if (strlen(clientinfo->clientstring))
	newpacket.commandlen += strlen(clientinfo->clientstring)+4;
      newpacket.commandlen += 6+6+6; 
      if (strlen(clientinfo->country))
	newpacket.commandlen += strlen(clientinfo->country)+4;
      if (strlen(clientinfo->lang))
	newpacket.commandlen += strlen(clientinfo->lang)+4;
    }

  newpacket.data = (char *) calloc (1,  newpacket.commandlen );
  newpacket.lock = 1;
  newpacket.type = 0x01;

  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0000);
  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0001);
  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0001);
  curbyte += aimutil_put16(newpacket.data+curbyte, strlen(sn));
  curbyte += aimutil_putstr(newpacket.data+curbyte, sn, strlen(sn));

  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0002);
  curbyte += aimutil_put16(newpacket.data+curbyte, strlen(password));
  password_encoded = (char *) malloc(strlen(password));
  aim_encode_password(password, password_encoded);
  curbyte += aimutil_putstr(newpacket.data+curbyte, password_encoded, strlen(password));
  free(password_encoded);
  
  curbyte += aim_puttlv_16(newpacket.data+curbyte, 0x0016, 0x0001);

  if (clientinfo)
    {
      if (strlen(clientinfo->clientstring))
	{
	  curbyte += aimutil_put16(newpacket.data+curbyte, 0x0003);
	  curbyte += aimutil_put16(newpacket.data+curbyte, strlen(clientinfo->clientstring));
	  curbyte += aimutil_putstr(newpacket.data+curbyte, clientinfo->clientstring, strlen(clientinfo->clientstring));
	}
      curbyte += aim_puttlv_16(newpacket.data+curbyte, 0x0017, 0x0001);
      curbyte += aim_puttlv_16(newpacket.data+curbyte, 0x0018, 0x0001);
      curbyte += aim_puttlv_16(newpacket.data+curbyte, 0x001a, 0x0013);
      if (strlen(clientinfo->country))
	{
	  curbyte += aimutil_put16(newpacket.data+curbyte, 0x000e);
	  curbyte += aimutil_put16(newpacket.data+curbyte, strlen(clientinfo->country));
	  curbyte += aimutil_putstr(newpacket.data+curbyte, clientinfo->country, strlen(clientinfo->country));
	}
       if (strlen(clientinfo->lang))
	{
	  curbyte += aimutil_put16(newpacket.data+curbyte, 0x000f);
	  curbyte += aimutil_put16(newpacket.data+curbyte, strlen(clientinfo->lang));
	  curbyte += aimutil_putstr(newpacket.data+curbyte, clientinfo->lang, strlen(clientinfo->lang));
	}
    }

  curbyte += aim_puttlv_16(newpacket.data+curbyte, 0x0009, 0x0015);

  newpacket.lock = 0;
  aim_tx_enqueue(&newpacket);

  return 0;
}
#else
{

  /* this is for the client info field of this packet.  for now, just
     put a few zeros in there and hope they don't notice. */
  char info_field[] = {
    0x00, 0x00, 0x00, 0x00
  };
  int info_field_len = 4;

  char *password_encoded = NULL;  /* to store encoded password */
  int n = 0; /* counter during packet construction */

  struct command_tx_struct newpacket;

  if (conn)
    newpacket.conn = conn;
  else
    newpacket.conn = aim_getconn_type(AIM_CONN_TYPE_AUTH);

  /* breakdown of new_packet_login_len */
  newpacket.commandlen = 6; /* SNAC: fixed bytes */
  newpacket.commandlen += 2; /* SN len */
  newpacket.commandlen += strlen(sn); /* SN text */
  newpacket.commandlen += 1; /* SN null terminator */
  newpacket.commandlen += 1; /* fixed byte */
  newpacket.commandlen += 2; /* password len */
  newpacket.commandlen += strlen(password); /* password text */
  newpacket.commandlen += 1; /* password null term*/
  newpacket.commandlen += 1; /* fixed byte */
  newpacket.commandlen += 2; /* info field len */
  newpacket.commandlen += info_field_len; /* info field text */
  newpacket.commandlen += 1; /* info field null term */
  newpacket.commandlen += 41; /* fixed bytes */

  /* allocate buffer to use for constructing packet_login */
  newpacket.data = (char *) malloc ( newpacket.commandlen );
  memset(newpacket.data, 0x00, newpacket.commandlen);

  newpacket.lock = 1;
  newpacket.type = 0x01;

  newpacket.data[0] = 0x00;
  newpacket.data[1] = 0x00;
  newpacket.data[2] = 0x00;
  newpacket.data[3] = 0x01;
  newpacket.data[4] = 0x00;
  newpacket.data[5] = 0x01;

  newpacket.data[6] = (char) ( (strlen(sn)) >> 8);
  newpacket.data[7] = (char) ( (strlen(sn)) & 0xFF);

  n = 8;
  memcpy(&(newpacket.data[n]), sn, strlen(sn));
  n += strlen(sn);
  newpacket.data[n] = 0x00;
  n++;

  newpacket.data[n] = 0x02;
  n++;

  /* store password length as word */
  newpacket.data[n] = (char) ( (strlen(password)) >> 8);
  newpacket.data[n+1] = (char) ( (strlen(password)) & 0xFF);
  n += 2;

  /* allocate buffer for encoded password */
  password_encoded = (char *) malloc(strlen(password));
  /* encode password */
  aim_encode_password(password, password_encoded);
  /* store encoded password */
  memcpy(&(newpacket.data[n]), password_encoded, strlen(password));

  n += strlen(password);
  /* free buffer */
  free(password_encoded);
  /* place null terminator after encoded password */
  newpacket.data[n] = 0x00;
  n++;

  newpacket.data[n] = 0x03;
  n++;

  newpacket.data[n] = (char) ( (info_field_len) >> 8);
  newpacket.data[n+1] = (char) ( (info_field_len) & 0xFF);
  n += 2;
  memcpy(&(newpacket.data[n]), info_field, info_field_len);
  n += info_field_len;
  newpacket.data[n] = 0x00;
  n++;

  newpacket.data[n] = 0x16;
  newpacket.data[n+1] = 0x00;
  newpacket.data[n+2] = 0x02;
  newpacket.data[n+3] = 0x00;
  n += 4;
  newpacket.data[n] = 0x01;
  newpacket.data[n+1] = 0x00;
  newpacket.data[n+2] = 0x17;
  newpacket.data[n+3] = 0x00;
  n += 4;

  newpacket.data[n] = 0x02;
  newpacket.data[n+1] = 0x00;
  newpacket.data[n+2] = 0x01;
  newpacket.data[n+3] = 0x00;
  n += 4;

  newpacket.data[n] = 0x18;
  newpacket.data[n+1] = 0x00;
  newpacket.data[n+2] = 0x02;
  newpacket.data[n+3] = 0x00;
  n += 4;

  newpacket.data[n] = 0x01;
  newpacket.data[n+1] = 0x00;
  newpacket.data[n+2] = 0x1a;
  newpacket.data[n+3] = 0x00;
  n += 4;

  newpacket.data[n] = 0x02;
  newpacket.data[n+1] = 0x00;
  newpacket.data[n+2] = 0x13;
  newpacket.data[n+3] = 0x00;
  n += 4;

  newpacket.data[n] = 0x0e;
  newpacket.data[n+1] = 0x00;
  newpacket.data[n+2] = 0x02;
  newpacket.data[n+3] = 0x75;
  n += 4;

  newpacket.data[n] = 0x73;
  newpacket.data[n+1] = 0x00;
  newpacket.data[n+2] = 0x0f;
  newpacket.data[n+3] = 0x00;
  n += 4;

  newpacket.data[n] = 0x02;
  newpacket.data[n+1] = 0x65;
  newpacket.data[n+2] = 0x6e;
  newpacket.data[n+3] = 0x00;
  n += 4;
  newpacket.data[n] = 0x09;
  newpacket.data[n+1] = 0x00;
  newpacket.data[n+2] = 0x02;
  newpacket.data[n+3] = 0x00;
  n += 4;

  newpacket.data[n] = 0x15;
  n += 1;

  aim_tx_enqueue(&newpacket);

  return 0;
}
#endif

/*
 *  int encode_password(
 *                      const char *password,
 *	                char *encoded
 *	                ); 
 *
 * This takes a const pointer to a (null terminated) string
 * containing the unencoded password.  It also gets passed
 * an already allocated buffer to store the encoded password.
 * This buffer should be the exact length of the password without
 * the null.  The encoded password buffer IS NOT NULL TERMINATED.
 *
 * The encoding_table seems to be a fixed set of values.  We'll
 * hope it doesn't change over time!  
 *
 */
int aim_encode_password(const char *password, char *encoded)
{
  u_char encoding_table[] = {
    0xf3, 0xb3, 0x6c, 0x99,
    0x95, 0x3f, 0xac, 0xb6,
    0xc5, 0xfa, 0x6b, 0x63,
    0x69, 0x6c, 0xc3, 0x9f
  };

  int i;
  
  for (i = 0; i < strlen(password); i++)
      encoded[i] = (password[i] ^ encoding_table[i]);

  return 0;
}




