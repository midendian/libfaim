/*
 * Administrative things for libfaim.
 *
 *  
 */

#include <faim/aim.h>

faim_export char *aim_getbuilddate(void)
{
  return AIM_BUILDDATE;
}

faim_export char *aim_getbuildtime(void)
{
  return AIM_BUILDTIME;
}

faim_export char *aim_getbuildstring(void)
{
  static char string[100];

  snprintf(string, 99, "%d.%d.%d-%s%s", 
	   FAIM_VERSION_MAJOR,
	   FAIM_VERSION_MINOR,
	   FAIM_VERSION_MINORMINOR,
	   aim_getbuilddate(),
	   aim_getbuildtime());
  return string;
}

#if debug > 0
faim_internal void faimdprintf(int dlevel, const char *format, ...)
{
  if (dlevel >= debug) {
    va_list ap;
    
    va_start(ap, format);
    vfprintf(stderr, format, ap);
    va_end(ap);
  }
  return;
}
#else
faim_internal void faimdprintf(int dlevel, const char *format, ...)
{
  return;
}
#endif
