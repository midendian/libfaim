
/*

  SNAC-related dodads... 

  outstanding_snacs is a list of aim_snac_t structs.  A SNAC should be added
  whenever a new SNAC is sent and it should remain in the list until the
  response for it has been receieved.

 */

#include <aim.h>

struct aim_snac_t *aim_outstanding_snacs = NULL;
u_long aim_snac_nextid = 0x00000001;

u_long aim_newsnac(struct aim_snac_t *newsnac)
{
  struct aim_snac_t *local = NULL;
  
  local = (struct aim_snac_t *)malloc(sizeof(struct aim_snac_t));
  memcpy(local, newsnac, sizeof(struct aim_snac_t));
  local->next = NULL;
  local->issuetime = time(&local->issuetime);

  if (aim_outstanding_snacs!=NULL)
    {
      struct aim_snac_t *cur = aim_outstanding_snacs;
      
      if (cur->next == NULL)
	{
	  cur->next = local;
	}
      else
	{
	  for (;cur->next!=NULL; cur=cur->next)
	    ;
	  cur->next = local;
	}
    }
  else
    {
      aim_outstanding_snacs = local;
      aim_outstanding_snacs->next = NULL;
    }

  return local->id;
}

/* FIXME: there's a bug in here... just don't have more than two outstanding
          SNACs and you'll be ok */
struct aim_snac_t *aim_remsnac(u_long id)
{
  struct aim_snac_t *cur = aim_outstanding_snacs;

  if(cur)
    {
      if (cur->next)
	{
	  struct aim_snac_t *ret = NULL;
	  for(;(cur->next!=NULL) && (cur->id != id);cur=cur->next)
	    ;
	  if (cur->id == id)
	    {
	      ret = cur;
	      cur->next = NULL;
	      return ret;
	    }
	  else 
	    {
	      return NULL;
	    }
	}
      else
	{
	  aim_outstanding_snacs = NULL;
	  return cur;
	}
    }
  else
    return NULL;
}


/*
  This is for cleaning up old SNACs that either don't get replies or
  a reply was never received for.  Garabage collection. Plain and simple.

  maxage is the _minimum_ age in seconds to keep SNACs

 */
int aim_cleansnacs(int maxage)
{
  struct aim_snac_t *cur = aim_outstanding_snacs;
  time_t curtime;
  
  curtime = time(&curtime);

  while (cur)
    {
#if 1
      if ( (cur) && (((cur->issuetime) + maxage) < curtime))
	{
	  printf("aimsnac: WARNING purged obsolete snac %ul\n", (unsigned int) cur->id);
#if 1
	  aim_remsnac(cur->id);
#endif
	}
#endif
      cur = cur->next;
    }

  return 0;
}
