/*
 * Cookie Caching stuff. Adam wrote this, apparently just some
 * derivatives of n's SNAC work. I cleaned it up, added comments.
 * 
 * I'm going to rewrite this stuff eventually, honest. -jbm
 * 
 */

/*
 * I'm assuming that cookies are type-specific. that is, we can have
 * "1234578" for type 1 and type 2 concurrently. if i'm wrong, then we
 * lose some error checking. if we assume cookies are not type-specific and are
 * wrong, we get quirky behavior when cookies step on each others' toes.
 */

#define FAIM_INTERNAL
#include <faim/aim.h>

/*
 * aim_cachecookie: 
 * appends a cookie to the cookie list for sess.
 * - if cookie->cookie for type cookie->type is found, -1 is returned
 * - copies cookie struct; you need to free() it afterwards;
 * - cookie->data is not copied, but passed along. don't free it.
 * - cookie->type is just passed across.
 * 
 * returns -1 on error, 0 on success.  
 */
faim_internal int aim_cachecookie(struct aim_session_t *sess,
				  struct aim_msgcookie_t *cookie)
{
  struct aim_msgcookie_t *newcook;

  if (!sess || !cookie)
    return -1;

  printf("\t\tCC cache %d %s", cookie->type, cookie->cookie);
  if(cookie->type == AIM_COOKIETYPE_OFTGET) {
    struct aim_filetransfer_priv *priv;
    priv = cookie->data;
    printf("%s\n", priv->sn);
  } else
    printf("\n");

  if( (newcook = aim_checkcookie(sess, cookie->cookie, cookie->type)) ) {
    printf("aim_cachecookie: cookie already cached\n");
    return -1;
  }
  
  if (!(newcook = malloc(sizeof(struct aim_msgcookie_t))))
    return -1;
  memcpy(newcook, cookie, sizeof(struct aim_msgcookie_t));
  
  newcook->next = sess->msgcookies;
  sess->msgcookies = newcook;

  return 0;
}

/*
 * aim_uncachecookie:
 * takes a cookie string and grabs the cookie struct associated with
 * it. removes struct from chain.  returns the struct if found, or
 * NULL on not found.
 */
faim_internal struct aim_msgcookie_t *aim_uncachecookie(struct aim_session_t *sess, unsigned char *cookie, int type)
{
  struct aim_msgcookie_t *cur, **prev;

  if (!cookie || !sess->msgcookies)
    return NULL;

  printf("\t\tCC uncache %d %s\n", type, cookie);

  for (prev = &sess->msgcookies; (cur = *prev); ) {
    if ((cur->type == type) && 
	(memcmp(cur->cookie, cookie, 8) == 0)) {
      *prev = cur->next;
      return cur;
    }
    prev = &cur->next;
  }

  return NULL;
}

faim_internal struct aim_msgcookie_t *aim_mkcookie(unsigned char *c, int type, void *data) 
{
  struct aim_msgcookie_t *cookie;

  if (!c)
    return NULL;

  if (!(cookie = calloc(1, sizeof(struct aim_msgcookie_t))))
    return NULL;
  
  cookie->data = data;
  cookie->type = type;
  memcpy(cookie->cookie, c, 8);
  
  return cookie;
}
  
faim_internal struct aim_msgcookie_t *aim_checkcookie(struct aim_session_t *sess, const unsigned char *cookie, const int type)
{
  struct aim_msgcookie_t *cur;

  printf("\t\tCC check %d %s\n", type, cookie);  

  for (cur = sess->msgcookies; cur; cur = cur->next) {
    if ((cur->type == type) && 
	(memcmp(cur->cookie, cookie, 8) == 0))
      return cur;   
  }

  return NULL;
}

faim_internal int aim_freecookie(struct aim_session_t *sess, struct aim_msgcookie_t *cookie) {
  struct aim_msgcookie_t *cur, **prev;

  if (!sess || !cookie)
    return -1;

  /* 
   * Make sure its not in the list somewhere still.
   *
   * If this actually happens, theres been a major coding failure 
   * on my part. However, that does not reduce its occurance likelyhood.
   */
  for (prev = &sess->msgcookies; (cur = *prev); ) {
    if (cur == cookie) {
      *prev = cur->next;
    } else
      prev = &cur->next;
  }

  free(cookie);

  return 0;
} 

faim_internal int aim_msgcookie_gettype(int reqclass) {
  /* XXX: hokey-assed. needs fixed. */
  switch(reqclass) {
  case AIM_CAPS_BUDDYICON:
    return AIM_COOKIETYPE_OFTICON;
    break;
  case AIM_CAPS_VOICE:
    return AIM_COOKIETYPE_OFTVOICE;
    break;
  case AIM_CAPS_IMIMAGE:
    return AIM_COOKIETYPE_OFTIMAGE;
    break;
  case AIM_CAPS_CHAT:
    return AIM_COOKIETYPE_CHAT;
    break;
  case AIM_CAPS_GETFILE:
    return AIM_COOKIETYPE_OFTGET;
    break;
  case AIM_CAPS_SENDFILE:
    return AIM_COOKIETYPE_OFTSEND;
    break;
  default:
    return AIM_COOKIETYPE_UNKNOWN;
    break;
  }           
}
