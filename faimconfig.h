/*
 *  faimconfig.h
 *
 * Contains various compile-time options that apply _only to libfaim.
 * Note that setting any of these options in a frontend header does not imply
 * that they'll get set here.  Notably, the 'debug' of this file is _not_ 
 * the same as the frontend 'debug'.  They can be different values.
 *
 */

#ifndef __FAIMCONFIG_H__
#define __FAIMCONFIG_H__

/* 
 * set debug to be > 0 if you want debugging information spewing
 * on the attached tty.  set to 0 for daily use.  this value
 * is _not_ inherited by the frontend, only this backend.
 *
 * Default: 0  
*/
#define debug 0 

/*
 * Maximum number of connections the library can simultaneously
 * handle.  Five is fairly arbitrary.  Only one client that I 
 * know of uses more than one concurrently anyway (which means
 * its only lightly tested too).  
 *
 * Default: 5
 *
 */
#define AIM_CONN_MAX 5 

/*
 *  define TIS_TELNET_PROXY if you have a TIS firewall (Gauntlet) and
 * you want to use FAIM through the firewall
 *
 * XXX: The TIS firewall code hasn't existed, let alone worked,
 *      in many, many months.  I'd be happy to take fixes.
 *
 * Default: undefined
 */
#undef TIS_TELNET_PROXY "proxy.mydomain.com"

/*
 * USE_SNAC_FOR_IMS is an old feature that allowed better
 * tracking of error messages by caching SNAC IDs of outgoing
 * ICBMs and comparing them to incoming errors.  However,
 * its a helluvalot of overhead for something that should
 * rarely happen.  
 *
 * Default: undefined.  Hasn't been tested in quite a long
 * time.  Most likely doesn't even compile.  
 *
 */
#undef USE_SNAC_FOR_IMS

/*
 * Default Authorizer server name and TCP port for the OSCAR farm.  
 *
 * You shouldn't need to change this unless you're writing
 * your own server. 
 *
 * Note that only one server is needed to start the whole
 * AIM process.  The later server addresses come from
 * the authorizer service.
 *
 */
#define FAIM_LOGIN_SERVER "login.oscar.aol.com"
#define FAIM_LOGIN_PORT 5190

/*
 * MAX_READ_ERROR can be decreased if you find dead connections
 * lingering around, and not getting detected, for too long.
 *
 * Default: 100
 *
 */
#define MAX_READ_ERROR 100

#endif /* __FAIMCONFIG_H__ */
