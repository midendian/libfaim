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

  u_short maxf;
  u_short maxs;

  /* XXX: this is ugly. and big just for debugging. */
  char *literals[14][25] = {
    {"Invalid", 
     NULL
    },
    {"General", 
     "Invalid",
     "Error",
     "Client Ready",
     "Server Ready",
     "Service Request",
     "Redirect",
     "Rate Information Request",
     "Rate Information",
     "Rate Information Ack",
     NULL,
     "Rate Information Change",
     "Server Pause",
     NULL,
     "Server Resume",
     "Request Personal User Information",
     "Personal User Information",
     "Evil Notification",
     NULL,
     "Migration notice",
     "Message of the Day",
     "Set Privacy Flags",
     "Well Known URL",
     "NOP"
    },
    {"Location", 
      "Invalid",
      "Error",
      "Request Rights",
      "Rights Information", 
      "Set user information", 
      "Request User Information", 
      "User Information", 
      "Watcher Sub Request",
      "Watcher Notification"
    },
    {"Buddy List Management", 
      "Invalid", 
      "Error", 
      "Request Rights",
      "Rights Information",
      "Add Buddy", 
      "Remove Buddy", 
      "Watcher List Query", 
      "Watcher List Response", 
      "Watcher SubRequest", 
      "Watcher Notification", 
      "Reject Notification", 
      "Oncoming Buddy", 
      "Offgoing Buddy"
    },
    {"Messeging", 
      "Invalid",
      "Error", 
      "Add ICBM Parameter",
      "Remove ICBM Parameter", 
      "Request Parameter Information",
      "Parameter Information",
      "Outgoing Message", 
      "Incoming Message",
      "Evil Request",
      "Evil Reply", 
      "Missed Calls",
      "Message Error", 
      "Host Ack"
    },
    {"Advertisements", 
      "Invalid", 
      "Error", 
      "Request Ad",
      "Ad Data (GIFs)"
    },
    {"Invitation / Client-to-Client", 
     "Invalid",
     "Error",
     "Invite a Friend",
     "Invitation Ack"
    },
    {"Administrative", 
      "Invalid",
      "Error",
      "Information Request",
      "Information Reply",
      "Information Change Request",
      "Information Chat Reply",
      "Account Confirm Request",
      "Account Confirm Reply",
      "Account Delete Request",
      "Account Delete Reply"
    },
    {"Popups", 
      "Invalid",
      "Error",
      "Display Popup"
    },
    {"BOS", 
      "Invalid",
      "Error",
      "Request Rights",
      "Rights Response",
      "Set group permission mask",
      "Add permission list entries",
      "Delete permission list entries",
      "Add deny list entries",
      "Delete deny list entries",
      "Server Error"
    },
    {"User Lookup", 
      "Invalid",
      "Error",
      "Search Request",
      "Search Response"
    },
    {"Stats", 
      "Invalid",
      "Error",
      "Set minimum report interval",
      "Report Events"
    },
    {"Translate", 
      "Invalid",
      "Error",
      "Translate Request",
      "Translate Reply",
    },
    {"Chat Navigation", 
      "Invalid",
      "Error",
      "Request rights",
      "Request Exchange Information",
      "Request Room Information",
      "Request Occupant List",
      "Search for Room",
      "Outgoing Message", 
      "Incoming Message",
      "Evil Request", 
      "Evil Reply", 
      "Chat Error",
    }
  };

  maxf = sizeof(literals) / sizeof(literals[0]);
  maxs = sizeof(literals[0]) / sizeof(literals[0][0]);

  family = aimutil_get16(workingPtr->data+0);
  subtype= aimutil_get16(workingPtr->data+2);

  if((family < maxf) && (subtype+1 < maxs) && (literals[family][subtype] != NULL))
    printf("bleck: null handler for %04x/%04x (%s)\n", family, subtype, literals[family][subtype+1]);
  else
    printf("bleck: null handler for %04x/%04x (no literal)\n",family,subtype);

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

  faimdprintf(1, "aim_conn_addhandler: adding for %04x/%04x\n", family, type);

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

  faimdprintf(1, "aim_callhandler: calling for %04x/%04x\n", family, type);
  
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
  return 1; /* XXX */
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
  
  if (sess->queue_incoming == NULL) {
    faimdprintf(1, "parse_generic: incoming packet queue empty.\n");
    return 0;
  } else {
    workingPtr = sess->queue_incoming;
    for (i = 0; workingPtr != NULL; workingPtr = workingPtr->next, i++) {
      /*
       * XXX: This is still fairly ugly.
       */
      if (workingPtr->handled)
	continue;

      switch(workingPtr->conn->type) {
      case -1:
	/*
	 * This can happen if we have a queued command
	 * that was recieved after a connection has 
	 * been terminated.  In which case, the handler
	 * list has been cleared, and there's nothing we
	 * can do for it.  We can only cancel it.
	 */
	workingPtr->handled = 1;
	break;
      case AIM_CONN_TYPE_AUTH: {
	u_long head;
	
	head = aimutil_get32(workingPtr->data);
	if (head == 0x00000001) {
	  faimdprintf(1, "got connection ack on auth line\n");
	  workingPtr->handled = 1;
	} else {
	  u_short family,subtype;
	  
	  family = aimutil_get16(workingPtr->data);
	  subtype = aimutil_get16(workingPtr->data+2);
	  
	  switch (family) {
	    /* New login protocol */
#ifdef SNACLOGIN
	  case 0x0017:
	    if (subtype == 0x0001)
	      workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0017, 0x0001, workingPtr);
	    else if (subtype == 0x0003)
	      workingPtr->handled = aim_authparse(sess, workingPtr);
	    else if (subtype == 0x0007)
	      workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0017, 0x0007, workingPtr);
	    else
	      workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0017, 0xffff, workingPtr);
	    break;
#else	
	    /* XXX: this isnt foolproof */
	  case 0x0001:
	    if (subtype == 0x0003)
	      workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_GEN, AIM_CB_GEN_SERVERREADY, workingPtr);
	    else
	      workingPtr->handled = aim_authparse(sess, workingPtr);
	    break;
	  case 0x0007:
	    if (subtype == 0x0005)
	      workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_ADM, AIM_CB_ADM_INFOCHANGE_REPLY, workingPtr);
	    break;
	  default:
	    /* Old login protocol */
	    /* any user callbacks will be called from here */
	    workingPtr->handled = aim_authparse(sess, workingPtr);
#endif
	  }
	}
	break;
      }
      case AIM_CONN_TYPE_BOS: {
	u_short family;
	u_short subtype;

	if (workingPtr->type == 0x04) {
	  workingPtr->handled = aim_negchan_middle(sess, workingPtr);
	  break;
	}

	family = aimutil_get16(workingPtr->data);
	subtype = aimutil_get16(workingPtr->data+2);
	
	switch (family) {
	case 0x0000: /* not really a family, but it works */
	  if (subtype == 0x0001)
	    workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0000, 0x0001, workingPtr);
	  else
	    workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_UNKNOWN, workingPtr);
	  break;
	case 0x0001: /* Family: General */
	  switch (subtype) {
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
	    workingPtr->handled = aim_parsemotd_middle(sess, workingPtr);
	    break;
	  default:
	    workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_GEN, AIM_CB_GEN_DEFAULT, workingPtr);
	    break;
	  }
	case 0x0002: /* Family: Location */
	  switch (subtype) {
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
	    break;
	  }
	case 0x0003: /* Family: Buddy List */
	  switch (subtype) {
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
	  switch (subtype) {
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
	  switch (subtype) {
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
	break;
      }
      case AIM_CONN_TYPE_CHATNAV: {
	u_short family;
	u_short subtype;
	family = aimutil_get16(workingPtr->data);
	subtype= aimutil_get16(workingPtr->data+2);
	
	if ((family == 0x0002) && (subtype == 0x0006)) {
	  workingPtr->handled = 1;
	  aim_conn_setstatus(workingPtr->conn, AIM_CONN_STATUS_READY);
	} else if ((family == 0x000d) && (subtype == 0x0009)) {
	  workingPtr->handled = aim_chatnav_parse_info(sess, workingPtr);
	} else {
	  workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, family, subtype, workingPtr);
	}
	break;
      }
      case AIM_CONN_TYPE_CHAT: {
	u_short family, subtype;
	
	family = aimutil_get16(workingPtr->data);
	subtype= aimutil_get16(workingPtr->data+2);
	
	if ((family == 0x0000) && (subtype == 0x00001))
	  workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0000, 0x0001, workingPtr);
	else if (family == 0x0001) {
	  if (subtype == 0x0001)
	    workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0001, 0x0001, workingPtr);
	  else if (subtype == 0x0003)
	    workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0001, 0x0003, workingPtr);
	  else if (subtype == 0x0007)
	    workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, 0x0001, 0x0007, workingPtr);
	  else
	    printf("Chat: unknown snac %04x/%04x\n", family, subtype);
	} else if (family == 0x000e) {
	  if (subtype == 0x0002)
	    workingPtr->handled = aim_chat_parse_infoupdate(sess, workingPtr);
	  else if (subtype == 0x0003)
	    workingPtr->handled = aim_chat_parse_joined(sess, workingPtr);	
	  else if (subtype == 0x0004)
	    workingPtr->handled = aim_chat_parse_leave(sess, workingPtr);	
	  else if (subtype == 0x0006)
	    workingPtr->handled = aim_chat_parse_incoming(sess, workingPtr);
	  else	
	    printf("Chat: unknown snac %04x/%04x\n", family, subtype); 
	} else {
	  printf("Chat: unknown snac %04x/%04x\n", family, subtype);
	  workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_CHT, AIM_CB_CHT_DEFAULT, workingPtr);
	}
	break;
      }
      default:
	printf("\ninternal error: unknown connection type (very bad.) (type = %d, fd = %d, channel = %02x, commandlen = %02x)\n\n", workingPtr->conn->type, workingPtr->conn->fd, workingPtr->type, workingPtr->commandlen);
	workingPtr->handled = aim_callhandler_noparam(sess, workingPtr->conn, AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_UNKNOWN, workingPtr);
	break;
      }	
    }
  }

  /* 
   * This doesn't have to be called here.  It could easily be done
   * by a seperate thread or something. It's an administrative operation,
   * and can take a while. Though the less you call it the less memory
   * you'll have :)
   */
  aim_purge_rxqueue(sess);
  
  return 0;
}

int aim_parsemotd_middle(struct aim_session_t *sess,
			      struct command_rx_struct *command, ...)
{
  rxcallback_t userfunc = NULL;
  char *msg;
  int ret=1;
  struct aim_tlvlist_t *tlvlist;
  u_short id;

  /*
   * Dunno.
   */
  id = aimutil_get16(command->data+10);

  /* 
   * TLVs follow 
   */
  tlvlist = aim_readtlvchain(command->data+12, command->commandlen-12);
  
  msg = aim_gettlv_str(tlvlist, 0x000b, 1);
  
  userfunc = aim_callhandler(command->conn, 0x0001, 0x0013);
  if (userfunc)
    ret =  userfunc(sess, command, id, msg);

  aim_freetlvchain(&tlvlist);

  return ret;
  
}

int aim_handleredirect_middle(struct aim_session_t *sess,
			      struct command_rx_struct *command, ...)
{
  struct aim_tlv_t *tmptlv = NULL;
  int serviceid = 0x00;
  char cookie[AIM_COOKIELEN];
  char *ip = NULL;
  rxcallback_t userfunc = NULL;
  struct aim_tlvlist_t *tlvlist;
  int ret = 1;
  
  if (!(tlvlist = aim_readtlvchain(command->data+10, command->commandlen-10)))
    {
      printf("libfaim: major bug: unable to read tlvchain from redirect\n");
      return ret;
    }
  
  if (!(tmptlv = aim_gettlv(tlvlist, 0x000d, 1))) 
    {
      printf("libfaim: major bug: no service ID in tlvchain from redirect\n");
      aim_freetlvchain(&tlvlist);
      return ret;
    }
  serviceid = aimutil_get16(tmptlv->value);

  if (!(ip = aim_gettlv_str(tlvlist, 0x0005, 1))) 
    {
      printf("libfaim: major bug: no IP in tlvchain from redirect (service 0x%02x)\n", serviceid);
      aim_freetlvchain(&tlvlist);
      return ret;
    }
  
  if (!(tmptlv = aim_gettlv(tlvlist, 0x0006, 1)))
    {
      printf("libfaim: major bug: no cookie in tlvchain from redirect (service 0x%02x)\n", serviceid);
      aim_freetlvchain(&tlvlist);
      return ret;
    }
  memcpy(cookie, tmptlv->value, AIM_COOKIELEN);

  if (serviceid == AIM_CONN_TYPE_CHAT)
    {
      /*
       * Chat hack.
       *
       */
      userfunc = aim_callhandler(command->conn, 0x0001, 0x0005);
      if (userfunc)
	ret =  userfunc(sess, command, serviceid, ip, cookie, sess->pendingjoin);
      free(sess->pendingjoin);
      sess->pendingjoin = NULL;
    }
  else
    {
      userfunc = aim_callhandler(command->conn, 0x0001, 0x0005);
      if (userfunc)
	ret =  userfunc(sess, command, serviceid, ip, cookie);
    }

  /*
   * XXX: Is there a leak here?  Where does IP get freed?
   */
  aim_freetlvchain(&tlvlist);

  return ret;
}

int aim_parse_unknown(struct aim_session_t *sess,
		      struct command_rx_struct *command, ...)
{
  u_int i = 0;

  faimdprintf(1, "\nRecieved unknown packet:");

  for (i = 0; i < command->commandlen; i++)
    {
      if ((i % 8) == 0)
	printf("\n\t");

      printf("0x%2x ", command->data[i]);
    }
  
  printf("\n\n");

  return 1;
}


int aim_negchan_middle(struct aim_session_t *sess,
		       struct command_rx_struct *command)
{
  struct aim_tlvlist_t *tlvlist;
  char *msg = NULL;
  unsigned short code = 0;
  struct aim_tlv_t *tmptlv;
  rxcallback_t userfunc = NULL;
  int ret = 1;

  tlvlist = aim_readtlvchain(command->data, command->commandlen);

  if ((tmptlv = aim_gettlv(tlvlist, 0x0009, 1)))
    code = aimutil_get16(tmptlv->value);

  if ((tmptlv = aim_gettlv(tlvlist, 0x000b, 1)))
    msg = aim_gettlv_str(tlvlist, 0x000b, 1);

  userfunc = aim_callhandler(command->conn, 
			     AIM_CB_FAM_SPECIAL, AIM_CB_SPECIAL_CONNERR);
  if (userfunc)
    ret =  userfunc(sess, command, code, msg);

  aim_freetlvchain(&tlvlist);
  free(msg);

  return ret;
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



