/*
 * aim_logoff.c
 *
 *
 * XXX remove this excuse for a file.
 *
 */

#define FAIM_INTERNAL
#include <faim/aim.h> 

/* 
 * aim_logoff()
 * 
 * Closes -ALL- open connections.
 *
 */
faim_export int aim_logoff(struct aim_session_t *sess)
{
  aim_connrst(sess);  /* in case we want to connect again */

  return 0;

}
