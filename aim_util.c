/*
 *
 *
 *
 */

#include <faim/aim.h>

int aimutil_put8(u_char *buf, u_char data)
{
  buf[0] = (u_char)data&0xff;
  return 1;
}

u_char aimutil_get8(u_char *buf)
{
  return buf[0];
}

/*
 * Endian-ness issues here?
 */
int aimutil_put16(u_char *buf, u_short data)
{
  buf[0] = (u_char)(data>>8)&0xff;
  buf[1] = (u_char)(data)&0xff;
  return 2;
}

u_short aimutil_get16(u_char *buf)
{
  u_short val;
  val = (buf[0] << 8) & 0xff00;
  val+= (buf[1]) & 0xff;
  return val;
}

int aimutil_put32(u_char *buf, u_long data)
{
  buf[0] = (u_char)(data>>24)&0xff;
  buf[1] = (u_char)(data>>16)&0xff;
  buf[2] = (u_char)(data>>8)&0xff;
  buf[3] = (u_char)(data)&0xff;
  return 4;
}

u_long aimutil_get32(u_char *buf)
{
  u_long val;
  val = (buf[0] << 24) & 0xff000000;
  val+= (buf[1] << 16) & 0x00ff0000;
  val+= (buf[2] <<  8) & 0x0000ff00;
  val+= (buf[3]      ) & 0x000000ff;
  return val;
}

int aimutil_putstr(u_char *dest, const u_char *src, int len)
{
  memcpy(dest, src, len);
  return len;
}

/*
 * Tokenizing functions.  Used to portably replace strtok/sep.
 *   -- DMP.
 *
 */
int aimutil_tokslen(char *toSearch, int index, char dl)
{
  int curCount = 1;
  char *next;
  char *last;
  int toReturn;

  last = toSearch;
  next = strchr(toSearch, dl);
  
  while(curCount < index && next != NULL)
    {
      curCount++;
      last = next + 1;
      next = strchr(last, dl);
    }
  
  if ((curCount < index) || (next == NULL))
    toReturn = strlen(toSearch) - (curCount - 1);
  else
    toReturn = next - toSearch - (curCount - 1);

  return toReturn;
}

int aimutil_itemcnt(char *toSearch, char dl)
{
  int curCount;
  char *next;
  
  curCount = 1;
  
  next = strchr(toSearch, dl);
  
  while(next != NULL)
    {
      curCount++;
      next = strchr(next + 1, dl);
    }
  
  return curCount;
}

char *aimutil_itemidx(char *toSearch, int index, char dl)
{
  int curCount;
  char *next;
  char *last;
  char *toReturn;
  
  curCount = 0;
  
  last = toSearch;
  next = strchr(toSearch, dl);
  
  while(curCount < index && next != NULL)
    {
      curCount++;
      last = next + 1;
      next = strchr(last, dl);
    }
  
  if (curCount < index)
    {
      toReturn = malloc(sizeof(char));
      *toReturn = '\0';
    }
  next = strchr(last, dl);
  
  if (curCount < index)
    {
      toReturn = malloc(sizeof(char));
      *toReturn = '\0';
    }
  else
    {
      if (next == NULL)
	{
	  toReturn = malloc((strlen(last) + 1) * sizeof(char));
	  strcpy(toReturn, last);
	}
      else
	{
	  toReturn = malloc((next - last + 1) * sizeof(char));
	  memcpy(toReturn, last, (next - last));
	  toReturn[next - last] = '\0';
	}
    }
  return toReturn;
}
