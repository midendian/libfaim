#include <faim/aim.h>

struct aim_tlvlist_t *aim_readtlvchain(u_char *buf, int maxlen)
{
  int pos;
  struct aim_tlvlist_t *list;
  struct aim_tlvlist_t *cur;
  
  u_short type;
  u_short length;

  if (!buf)
    return NULL;

  list = NULL;
  
  pos = 0;

  while (pos < maxlen)
    {
      type = aimutil_get16(buf+pos);
      pos += 2;

      if (pos < maxlen)
	{
	  length = aimutil_get16(buf+pos);
	  pos += 2;
	  
	  if ((pos+length) <= maxlen)
	    {
	      cur = (struct aim_tlvlist_t *)malloc(sizeof(struct aim_tlvlist_t));
	      memset(cur, 0x00, sizeof(struct aim_tlvlist_t));

	      cur->tlv = aim_createtlv();	
	      cur->tlv->type = type;
	      cur->tlv->length = length;
	      cur->tlv->value = (u_char *)malloc(length*sizeof(u_char));
	      memcpy(cur->tlv->value, buf+pos, length);
	      
	      cur->next = list;
	      list = cur;
	      
	      pos += length;
	    }
	}
    }

  return list;
}

void aim_freetlvchain(struct aim_tlvlist_t **list)
{
  struct aim_tlvlist_t *cur, *cur2;

  if (!list || !(*list))
    return;

  cur = *list;
  while (cur)
    {
      aim_freetlv(&cur->tlv);
      cur2 = cur->next;
      free(cur);
      cur = cur2;
    }
  list = NULL;
  return;
}

/*
 * Grab the Nth TLV of type type in the TLV list list.
 */
struct aim_tlv_t *aim_gettlv(struct aim_tlvlist_t *list, u_short type, int nth)
{
  int i;
  struct aim_tlvlist_t *cur;
  
  i = 0;
  for (cur = list; cur != NULL; cur = cur->next)
    {
      if (cur && cur->tlv)
	{
	  if (cur->tlv->type == type)
	    i++;
	  if (i >= nth)
	    return cur->tlv;
	}
    }
  return NULL;
}

char *aim_gettlv_str(struct aim_tlvlist_t *list, u_short type, int nth)
{
  struct aim_tlv_t *tlv;
  char *newstr;

  if (!(tlv = aim_gettlv(list, type, nth)))
    return NULL;
  
  newstr = (char *) malloc(tlv->length + 1);
  memcpy(newstr, tlv->value, tlv->length);
  *(newstr + tlv->length) = '\0';

  return newstr;
}

struct aim_tlv_t *aim_grabtlv(u_char *src)
{
  struct aim_tlv_t *dest = NULL;

  dest = aim_createtlv();

  dest->type = src[0] << 8;
  dest->type += src[1];

  dest->length = src[2] << 8;
  dest->length += src[3];

  dest->value = (u_char *) malloc(dest->length*sizeof(u_char));
  memset(dest->value, 0, dest->length*sizeof(u_char));

  memcpy(dest->value, &(src[4]), dest->length*sizeof(u_char));
  
  return dest;
}

struct aim_tlv_t *aim_grabtlvstr(u_char *src)
{
  struct aim_tlv_t *dest = NULL;

  dest = aim_createtlv();

  dest->type = src[0] << 8;
  dest->type += src[1];

  dest->length = src[2] << 8;
  dest->length += src[3];

  dest->value = (u_char *) malloc((dest->length+1)*sizeof(u_char));
  memset(dest->value, 0, (dest->length+1)*sizeof(u_char));

  memcpy(dest->value, &(src[4]), dest->length*sizeof(u_char));
  dest->value[dest->length] = '\0';

  return dest;
}

int aim_puttlv (u_char *dest, struct aim_tlv_t *newtlv)
{
  int i=0;

  dest[i++] = newtlv->type >> 8;
  dest[i++] = newtlv->type & 0x00FF;
  dest[i++] = newtlv->length >> 8;
  dest[i++] = newtlv->length & 0x00FF;
  memcpy(&(dest[i]), newtlv->value, newtlv->length);
  i+=newtlv->length;
  return i;
}

struct aim_tlv_t *aim_createtlv(void)
{
  struct aim_tlv_t *newtlv = NULL;
  newtlv = (struct aim_tlv_t *)malloc(sizeof(struct aim_tlv_t));
  memset(newtlv, 0, sizeof(struct aim_tlv_t));
  return newtlv;
}

int aim_freetlv(struct aim_tlv_t **oldtlv)
{
  if (!oldtlv)
    return -1;
  if (!*oldtlv)
    return -1;
  if ((*oldtlv)->value)
    free((*oldtlv)->value);
  free(*(oldtlv));
  (*oldtlv) = NULL;

  return 0;
}

int aim_puttlv_16(u_char *buf, u_short t, u_short v)
{
  int curbyte=0;
  curbyte += aimutil_put16(buf+curbyte, (u_short)(t&0xffff));
  curbyte += aimutil_put16(buf+curbyte, (u_short)0x0002);
  curbyte += aimutil_put16(buf+curbyte, (u_short)(v&0xffff));
  return curbyte;
}

int aim_puttlv_32(u_char *buf, u_short t, u_long v)
{
  int curbyte=0;
  curbyte += aimutil_put16(buf+curbyte, (u_short)(t&0xffff));
  curbyte += aimutil_put16(buf+curbyte, (u_short)0x0004);
  curbyte += aimutil_put32(buf+curbyte, (u_long)(v&0xffffffff));
  return curbyte;
}

int aim_puttlv_str(u_char *buf, u_short t, u_short l, u_char *v)
{
  int curbyte;
  
  curbyte  = 0;
  curbyte += aimutil_put16(buf+curbyte, (u_short)(t&0xffff));
  curbyte += aimutil_put16(buf+curbyte, (u_short)(l&0xffff));
  if (v)
    memcpy(buf+curbyte, v, l);
  curbyte += l;
  return curbyte;
}
