/*
 * aim_rxhandlers.c
 *
 * This file contains most all of the incoming packet handlers, along
 * with aim_rxdispatch(), the Rx dispatcher.  Queue/list management is
 * actually done in aim_rxqueue.c.
 *
 */

#include <faim/aim.h>

/*
 * Bleck functions get called when there's no non-bleck functions
 * around to cleanup the mess...
 */
int bleck(struct aim_session_t *sess,struct command_rx_struct *workingPtr, ...)
{
  u_short family;
  u_short subtype;
  family = (workingPtr->data[0] << 8) + workingPtr->data[1];
  subtype = (workingPtr->data[2] << 8) + workingPtr->data[3];
  printf("bleck: null handler for %04x/%04x\n", family, subtype);
  return 1;
}

int aim_conn_addhandler(struct aim_session_t *sess,
			struct aim_conn_t *conn,
			u_short family,
			u_short type,
			rxcallback_t newhandler,
			u_short flags)
{
  struct aim_rxcblist_t *new,*cur;

  if (!conn)
    return -1;

#if debug > 0
  printf("aim_conn_addhandler: adding for %04x/%04x\n", family, type);
#endif

  new = (struct aim_rxcblist_t *)calloc(1, sizeof(struct aim_rxcblist_t));
  new->family = family;
  new->type = type;
  new->flags = flags;
  if (!newhandler)
    new->handler = &bleck;
  else
    new->handler = newhandler;
  new->next = NULL;
  
  cur = conn->handlerlist;
  if (!cur)
    conn->handlerlist = new;
  else 
    {
      while (cur->next)
	cur = cur->next;
      cur->next = new;
    }

  return 0;
}

int aim_clearhandlers(struct aim_conn_t *conn)
{
 struct aim_rxcblist_t *cur,*tmp;
 if (!conn)
   return -1;

 cur = conn->handlerlist;
 while(cur)
   {
     tmp = cur->next;
     free(cur);
     cur = tmp;
   }
 return 0;
}

rxcallback_t aim_callhandler(struct aim_conn_t *conn,
		    u_short family,
		    u_short type)
{
  struct aim_rxcblist_t *cur;

  if (!conn)
    return NULL;

#if debug > 0
  printf("aim_callhandler: calling for %04x/%04x\n", family, type);
#endif
  
  cur = conn->handlerlist;
  while(cur)
    {
      if ( (cur->family == family) && (cur->type == type) )
	return cur->handler;
      cur = cur->next;
    }

  if (type==0xffff)
    return NULL;
  return aim_callhandler(conn, family, 0xffff);
}

int aim_callhandler_noparam(struct aim_session_t *sess,
			    struct aim_conn_t *conn,
			    u_short family,
			    u_short type,
			    struct command_rx_struct *ptr)
{
  rxcallback_t userfunc = NULL;
  userfunc = aim_callhandler(conn, family, type);
  if (userfunc)
    return userfunc(sess, ptr);
  return 0;
}

/*
  aim_rxdispatch()

  Basically, heres what this should do:
    1) Determine correct packet handler for this packet
    2) Mark the packet handled (so it can be dequeued in purge_queue())
    3) Send the packet to the packet handler
    4) Go to next packet in the queue and start over
    5) When done, run purge_queue() to purge handled commands

  Note that any unhandlable packets should probably be left in the
  queue.  This is the best way to prevent data loss.  This means
  that a single packet may get looked at by this function multiple
  times.  This is more good than bad!  This behavior may change.

  Aren't queue's fun? 

  TODO: Get rid of all the ugly if's.
  TODO: Clean up.
  TODO: More support for mid-level handlers.
  TODO: Allow for NULL handlers.
  
 */
int aim_rxdispatch(struct aim_session_t *sess)
{
  int i = 0;
  struct command_rx_struct *workingPtr = NULL;
  
  if (sess->queue_incoming == NULL)
    /* this shouldn't really happen, unless the main loop's select is broke  */
    printf("parse_generic: incoming packet queue empty.\n");
  else
    {
      workingPtr = sess->queue_incoming;
      for (i = 0; workingPtr != NULL; i++)
	{
	  switch(workingPtr->conn->type)
	    {
	    case AIM_CONN_TYPE_AUTH:
	      {
		u_long head;

		head = aimutil_get32(workingPtr->data);
		if (head == 0x00000001)
		  {
#if debug > 0
		    printf("got connection ack on auth line\n");
#endif
		    workingPtr->handled = 1;
		  }
		else
		  {
		    /* any user callbacks will be called from here */
		    workingPtr->handled = aim_authparse(sess, workingPtr);
		  }
	      }
	      break;
	    case AIM_CONN_TYPE_BOS:
	      {
		u_short family;
		u_short subtype;

		family = aimutil_get16(workingPtr->data);
		subtype = aimutil_get16(workingPtr->data+2);

		switch (family)
		  {
		  case 0x0000: /* not really a family, but it works */
		    if (subtype == 0x0001)
		      workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0000, 0x0001, workingPtr);
		    else
		      workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_UNKNOWN, workingPtr);
		    break;
		  case 0x0001: /* Family: General */
		    switch (subtype)
		      {
		      case 0x0001:
			workingPtr->handled = aim_parse_generalerrs(sess, workingPtr);
			break;
		      case 0x0003:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0001, 0x0003, workingPtr);
			break;
		      case 0x0005:
			workingPtr->handled = aim_handleredirect_middle(sess, workingPtr);
			break;
		      case 0x0007:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0001, 0x0007, workingPtr);
			break;
		      case 0x000a:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0001, 0x000a, workingPtr);
			break;
		      case 0x000f:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0001, 0x000f, workingPtr);
			break;
		      case 0x0013:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0001, 0x0013, workingPtr);
			break;
		      default:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_GEN, AIM_CB_GEN_DEFAULT, workingPtr);
		      }
		    break;
		  case 0x0002: /* Family: Location */
		    switch (subtype)
		      {
		      case 0x0001:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0002, 0x0001, workingPtr);
			break;
		      case 0x0003:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0002, 0x0003, workingPtr);
			break;
		      case 0x0006:
			workingPtr->handled = aim_parse_userinfo_middle(sess, workingPtr);
			break;
		      default:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_LOC, AIM_CB_LOC_DEFAULT, workingPtr);
		      }
		    break;
		  case 0x0003: /* Family: Buddy List */
		    switch (subtype)
		      {
		      case 0x0001:
			workingPtr->handled = aim_parse_generalerrs(sess, workingPtr);
			break;
		      case 0x0003:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0003, 0x0003, workingPtr);
			break;
		      case 0x000b: /* oncoming buddy */
			workingPtr->handled = aim_parse_oncoming_middle(sess, workingPtr);
			break;
		      case 0x000c: /* offgoing buddy */
			workingPtr->handled = aim_parse_offgoing_middle(sess, workingPtr);
			break;
		      default:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_BUD, AIM_CB_BUD_DEFAULT, workingPtr);
		      }
		    break;
		  case 0x0004: /* Family: Messeging */
		    switch (subtype)
		      {
		      case 0x0001:
			workingPtr->handled = aim_parse_msgerror_middle(sess, workingPtr);
			break;
		      case 0x0005:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0004, 0x0005, workingPtr);
			break;
		      case 0x0007:
			workingPtr->handled = aim_parse_incoming_im_middle(sess, workingPtr);
			break;
		      case 0x000a:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0004, 0x000a, workingPtr);
			break;
		      default:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_MSG, AIM_CB_MSG_DEFAULT, workingPtr);
		      }
		    break;
		  case 0x0009:
		    if (subtype == 0x0001)
		      workingPtr->handled = aim_parse_generalerrs(sess, workingPtr);
		    else if (subtype == 0x0003)
		      workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0009, 0x0003, workingPtr);
		    else
		      workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_BOS, AIM_CB_BOS_DEFAULT, workingPtr);
		    break;
		  case 0x000a:  /* Family: User lookup */
		    switch (subtype)
		      {
		      case 0x0001:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x000a, 0x0001, workingPtr);
			break;
		      case 0x0003:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x000a, 0x0003, workingPtr);
			break;
		      default:
			workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_LOK, AIM_CB_LOK_DEFAULT, workingPtr);
		      }
		    break;
		  case 0x000b:
		    if (subtype == 0x0001)
		      workingPtr->handled = aim_parse_generalerrs(sess, workingPtr);
		    else if (subtype == 0x0002)
		      workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x000b, 0x0002, workingPtr);
		    else
		      workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_STS, AIM_CB_STS_DEFAULT, workingPtr);
		    break;
		  default:
		    workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_UNKNOWN, workingPtr);
		    break;
		  }
	      }
	      break;
	    case AIM_CONN_TYPE_CHATNAV:
	      {
		u_short family;
		u_short subtype;
		family = aimutil_get16(workingPtr->data);
		subtype= aimutil_get16(workingPtr->data+2);

		if ((family == 0x0002) && (subtype == 0x0006))
		  {
		    workingPtr->handled = 1;
		    aim_conn_setstatus(workingPtr->conn, AIM_CONN_STATUS_READY);
		  }
		else
		  {
		    workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, family, subtype, workingPtr);
		  }
	      }
	      break;
	    case AIM_CONN_TYPE_CHAT:
	      printf("\nAHH! Dont know what to do with CHAT stuff yet!\n");
	      workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_DEFAULT, workingPtr);
	      break;
	    default:
	      printf("\nAHHHHH! UNKNOWN CONNECTION TYPE! (0x%02x)\n\n", workingPtr->conn->type);
	      workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_UNKNOWN, workingPtr);
	      break;
	    }
	  /* move to next command */
	  workingPtr = workingPtr->next;
	}
    }

  sess->queue_incoming = aim_purge_rxqueue(sess->queue_incoming);
  
  return 0;
}

/*
 * TODO: check and cure memory leakage in this function.
 * TODO: update to use new tlvlist routines
 */
int aim_authparse(struct aim_session_t *sess, 
		  struct command_rx_struct *command)
{
  rxcallback_t userfunc = NULL;
  int iserror = 0;
  struct aim_tlv_t *tlv = NULL;
  char *errorurl = NULL;
  short errorcode = 0x00;
  u_int z = 0;
  u_short family,subtype;

  family = aimutil_get16(command->data);
  subtype= aimutil_get16(command->data+2);
  
  if ( (family == 0x0001) && 
       (subtype== 0x0003) )
    {
      /* "server ready"  -- can be ignored */
      userfunc = aim_callhandler(command->conn, AIM_CB_FAM_GEN, AIM_CB_GEN_SERVERREADY);
    }
  else if ( (family == 0x0007) &&
	    (subtype== 0x0005) )
    {
      /* "information change reply" */
      userfunc = aim_callhandler(command->conn, AIM_CB_FAM_ADM, AIM_CB_ADM_INFOCHANGE_REPLY);
    }
  else
    {
      /* anything else -- usually used for login; just parse as pure TLVs */

      /*
       * Free up the loginstruct first.
       */
      sess->logininfo.screen_name[0] = '\0';
      if (sess->logininfo.BOSIP)
	{
	  free(sess->logininfo.BOSIP);
	  sess->logininfo.BOSIP = NULL;
	}
      if (sess->logininfo.email)
	{
	  free(sess->logininfo.email);
	  sess->logininfo.email = NULL;
	}
      sess->logininfo.regstatus = 0;

      /* all this block does is figure out if it's an
	 error or a success, nothing more */
      while (z < command->commandlen)
	{
	  tlv = aim_grabtlvstr(&(command->data[z]));
	  switch(tlv->type) 
	    {
	    case 0x0001: /* screen name */
	      if (tlv->length >= MAXSNLEN)
		{
		  /* SN too large... truncate */
		  printf("faim: login: screen name from OSCAR too long! (%d)\n", tlv->length);
		  strncpy(sess->logininfo.screen_name, tlv->value, MAXSNLEN);
		}
	      else
		strncpy(sess->logininfo.screen_name, tlv->value, tlv->length);
	      z += 2 + 2 + tlv->length;
	      free(tlv);
	      tlv = NULL;
	      break;
	    case 0x0004: /* error URL */
	      errorurl = tlv->value;
	      z += 2 + 2 + tlv->length;
	      free(tlv);
	      tlv = NULL;
	      break;
	    case 0x0005: /* BOS IP */
	      sess->logininfo.BOSIP = tlv->value;
	      z += 2 + 2 + tlv->length;
	      free(tlv);
	      tlv = NULL;
	      break;
	    case 0x0006: /* auth cookie */
	      memcpy(sess->logininfo.cookie, tlv->value, AIM_COOKIELEN);
	      z += 2 + 2 + tlv->length;
	      free(tlv);
	      tlv=NULL;
	      break;
	    case 0x0011: /* email addy */
	      sess->logininfo.email = tlv->value;
	      z += 2 + 2 + tlv->length;
	      free(tlv);
	      tlv = NULL;
	      break;
	    case 0x0013: /* registration status */
	      sess->logininfo.regstatus = *(tlv->value);
	      z += 2 + 2 + tlv->length;
	      aim_freetlv(&tlv);
	      break;
	    case 0x0008: /* error code */
	      errorcode = *(tlv->value);
	      z += 2 + 2 + tlv->length;
	      aim_freetlv(&tlv);
	      iserror = 1;
	      break;
	    default:
	  z += 2 + 2 + tlv->length;
	  aim_freetlv(&tlv);
	  /* dunno */
	    }
	}

      if (iserror && 
	  errorurl)
	{
	  userfunc = aim_callhandler(command->conn, AIM_CB_FAM_GEN, AIM_CB_GEN_ERROR);
	  if (userfunc)
	    return userfunc(sess, command, errorurl, errorcode);
	  return 0;
	}
      else if (sess->logininfo.screen_name[0] && 
	       sess->logininfo.cookie[0] && sess->logininfo.BOSIP)
	{
	  userfunc = aim_callhandler(command->conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_AUTHSUCCESS);
	  if (userfunc)
	    return userfunc(sess, command);
	  return 0;
	}
      else
	userfunc = aim_callhandler(command->conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_AUTHOTHER);
    }

  if (userfunc)
    return userfunc(sess, command);
  printf("handler not available!\n");
  return 0;
}

/*
 * TODO: check for and cure any memory leaks here.
 */
int aim_handleredirect_middle(struct aim_session_t *sess,
			      struct command_rx_struct *command, ...)
{
  struct aim_tlv_t *tlv = NULL;
  u_int z = 10;
  int serviceid = 0x00;
  char *cookie = NULL;
  char *ip = NULL;
  rxcallback_t userfunc = NULL;

  while (z < command->commandlen)
    {
      tlv = aim_grabtlvstr(&(command->data[z]));
      switch(tlv->type)
	{
	case 0x000d:  /* service id */
	  aim_freetlv(&tlv);
	  /* regrab as an int */
	  tlv = aim_grabtlv(&(command->data[z]));
	  serviceid = aimutil_get16(tlv->value);
	  z += 2 + 2 + tlv->length;
	  aim_freetlv(&tlv);
	  break;
	case 0x0005:  /* service server IP */
	  ip = tlv->value;
	  z += 2 + 2 + tlv->length;
	  free(tlv);
	  tlv = NULL;
	  break;
	case 0x0006: /* auth cookie */
	  cookie = tlv->value;
	  z += 2 + 2 + tlv->length;
	  free(tlv);
	  tlv = NULL;
	  break;
	default:
	  /* dunno */
	  z += 2 + 2 + tlv->length;
	  aim_freetlv(&tlv);
	}
    }
  userfunc = aim_callhandler(command->conn, 0x0001, 0x0005);
  if (userfunc)
    return userfunc(sess, command, serviceid, ip, cookie);
  return 0;
}

int aim_parse_unknown(struct aim_session_t *sess,
		      struct command_rx_struct *command, ...)
{
  u_int i = 0;

  printf("\nRecieved unknown packet:");

  for (i = 0; i < command->commandlen; i++)
    {
      if ((i % 8) == 0)
	printf("\n\t");

      printf("0x%2x ", command->data[i]);
    }
  
  printf("\n\n");

  return 1;
}


/*
 * aim_parse_generalerrs()
 *
 * Middle handler for 0x0001 snac of each family.
 *
 */
int aim_parse_generalerrs(struct aim_session_t *sess,
			  struct command_rx_struct *command, ...)
{
  u_short family;
  u_short subtype;
  
  family = aimutil_get16(command->data+0);
  subtype= aimutil_get16(command->data+2);
  
  switch(family)
    {
    default:
      /* Unknown family */
      return aim_callhandler_noparam(sess, command->conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_UNKNOWN, command);
    }

  return 1;
}



