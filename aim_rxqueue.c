/*
  aim_rxqueue.c

  This file contains the management routines for the receive
  (incoming packet) queue.  The actual packet handlers are in
  aim_rxhandlers.c.

 */

#include <faim/aim.h> 


/*
 * This is a modified read() to make SURE we get the number
 * of bytes we are told to, otherwise block.
 *
 * Modified to count errno (Sébastien Carpe <scarpe@atos-group.com>)
 * 
*/
int aim_failsaferead(int fd, u_char *buf, int len)
{
  int i = 0;
  int j = 0;
  int err_count=0;
  
  while ((i < len) && (!(i < 0)))
    {
      j = read(fd, &(buf[i]), len-i);
      if ( (j < 0) && (errno != EAGAIN))
        return -errno; /* fail */
      else if (j==0) 
	{
	  err_count++;
	  if (err_count> MAX_READ_ERROR)  {
	    /*
	     * Reached maximum number of allowed read errors.
	     *
	     * Lets suppose the connection is lost and errno didn't
	     * know it.
	     *
	     */
          return (-1); 
	}
      } 
      else
        i += j; /* success, continue */
    }
  return i;
}

/*  
 * Grab as many command sequences as we can off the socket, and enqueue
 * each command in the incoming event queue in a seperate struct.
 */
int aim_get_command(struct aim_session_t *sess)
{
  int i, readgood, j, isav, err;
  int s;
  fd_set fds;
  struct timeval tv;
  char generic[6]; 
  struct command_rx_struct *workingStruct = NULL;
  struct aim_conn_t *conn = NULL;
  int selstat = 0;

  faimdprintf(1, "Reading generic/unknown response...");

  /* dont wait at all (ie, never call this unless something is there) */
  tv.tv_sec = 0; 
  tv.tv_usec = 0;
  conn = aim_select(sess, &tv, &selstat);

  if (conn==NULL) 
    return 0;  /* nothing waiting */

  s = conn->fd;

  if (s < 3) 
    return 0;

  FD_ZERO(&fds);
  FD_SET(s, &fds);
  tv.tv_sec = 0;  /* wait, but only for 10us */
  tv.tv_usec = 10;
  
  generic[0] = 0x00;  

  readgood = 0;
  i = 0;
  j = 0;
  /* read first 6 bytes (the FLAP header only) off the socket */
  while ( (select(s+1, &fds, NULL, NULL, &tv) == 1) && (i < 6))
    {
      if ((err = aim_failsaferead(s, &(generic[i]), 1)) < 0)
	{
	  /* error is probably not recoverable...(must be a pessimistic day) */
	  /* aim_conn_close(conn); */
	  return err;
   	}

      if (readgood == 0)
	{
	  if (generic[i] == 0x2a)
	  {
	    readgood = 1;
	    faimdprintf(1, "%x ", generic[i]);
	    i++;
	  }
	  else
	    {
	      faimdprintf(1, "skipping 0x%d ", generic[i]);
	      j++;
	    }
	}
      else
	{
	  faimdprintf(1, "%x ", generic[i]);
	  i++;
	}
      FD_ZERO(&fds);
      FD_SET(s, &fds);
      tv.tv_sec= 2;
      tv.tv_usec= 2;
    }

  /*
   * This shouldn't happen unless the socket breaks, the server breaks,
   * or we break.  We must handle it just in case.
   */
  if (generic[0] != 0x2a) {
    printf("Bad incoming data!");
    return -1;
  }	

  isav = i;

  /* allocate a new struct */
  workingStruct = (struct command_rx_struct *) malloc(sizeof(struct command_rx_struct));
  memset(workingStruct, 0x00, sizeof(struct command_rx_struct));

  workingStruct->lock = 1;  /* lock the struct */

  /* store channel -- byte 2 */
  workingStruct->type = (char) generic[1];

  /* store seqnum -- bytes 3 and 4 */
  workingStruct->seqnum = aimutil_get16(generic+2);

  /* store commandlen -- bytes 5 and 6 */
  workingStruct->commandlen = aimutil_get16(generic+4);

  workingStruct->nofree = 0; /* free by default */

  /* malloc for data portion */
  workingStruct->data = (u_char *) malloc(workingStruct->commandlen);

  /* read the data portion of the packet */
  if (aim_failsaferead(s, workingStruct->data, workingStruct->commandlen) < 0){
    aim_conn_close(conn);
    return -1;
  }

  faimdprintf(1, " done. (%db+%db read, %db skipped)\n", isav, i, j);

  workingStruct->conn = conn;

  workingStruct->next = NULL;  /* this will always be at the bottom */
  workingStruct->lock = 0; /* unlock */

  /* enqueue this packet */
  if (sess->queue_incoming == NULL) {
    sess->queue_incoming = workingStruct;
  } else {
    struct command_rx_struct *cur;

    /*
     * This append operation takes a while.  It might be faster
     * if we maintain a pointer to the last entry in the queue
     * and just update that.  Need to determine if the overhead
     * to maintain that is lower than the overhead for this loop.
     */
    for (cur = sess->queue_incoming; cur->next; cur = cur->next)
      ;
    cur->next = workingStruct;
  }
  
  workingStruct->conn->lastactivity = time(NULL);

  return 0;  
}

/*
 * Purge recieve queue of all handled commands (->handled==1).  Also
 * allows for selective freeing using ->nofree so that the client can
 * keep the data for various purposes.  
 *
 * If ->nofree is nonzero, the frame will be delinked from the global list, 
 * but will not be free'ed.  The client _must_ keep a pointer to the
 * data -- libfaim will not!  If the client marks ->nofree but
 * does not keep a pointer, it's lost forever.
 *
 */
void aim_purge_rxqueue(struct aim_session_t *sess)
{
  struct command_rx_struct *cur = NULL;
  struct command_rx_struct *tmp;

  if (sess->queue_incoming == NULL)
    return;
  
  if (sess->queue_incoming->next == NULL) {
    if (sess->queue_incoming->handled) {
      tmp = sess->queue_incoming;
      sess->queue_incoming = NULL;

      if (!tmp->nofree) {
	free(tmp->data);
	free(tmp);
      } else
	tmp->next = NULL;
    }
    return;
  }

  for(cur = sess->queue_incoming; cur->next != NULL; ) {
    if (cur->next->handled) {
      tmp = cur->next;
      cur->next = tmp->next;
      if (!tmp->nofree) {
	free(tmp->data);
	free(tmp);
      } else
	tmp->next = NULL;
    }	
    cur = cur->next;

    /* 
     * Be careful here.  Because of the way we just
     * manipulated the pointer, cur may be NULL and 
     * the for() will segfault doing the check unless
     * we find this case first.
     */
    if (cur == NULL)	
      break;
  }

  return;
}
