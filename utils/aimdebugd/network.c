/*
 * Misc network functions
 *
 */

#include <faim/aim.h>

#include <network.h>

int Read(int fd, unsigned char *buf, int len)
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
          if (err_count> 100)  {
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
 *  Create a listener socket.
 *
 *  We go out of our way to make this interesting. (The Easy Way is not
 *  thread-safe.)
 *
 */
int establish(u_short portnum)
{
  int listenfd;
  const int on = 1;
  struct addrinfo hints, *res, *ressave;
  char serv[5];
  
  sprintf(serv, "%d", portnum);

  memset(&hints, 0, sizeof(struct addrinfo));
  hints.ai_flags = AI_PASSIVE;
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = SOCK_STREAM;
  if (getaddrinfo(NULL/*any IP*/, serv, &hints, &res) != 0) {
    perror("getaddrinfo");
    return -1;
  }

  ressave = res;
  do { 
    listenfd = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    if (listenfd < 0)
      continue;
    setsockopt(listenfd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on));
    if (bind(listenfd, res->ai_addr, res->ai_addrlen) == 0)
      break; /* sucess */
    close(listenfd);
  } while ( (res = res->ai_next) );

  if (!res)
    return -1;

  if (listen(listenfd, 1024)!=0) {
    perror("listen");
    return -1;
  }

  freeaddrinfo(ressave);

  return listenfd;
}

int get_connection(int s)
{
  struct sockaddr isa;
  int addrlen = 0;
  int t;  /* socket of connection */
 
  memset(&isa, 0, sizeof(struct sockaddr));
  if ((t = accept(s,&isa,&addrlen)) < 0) {
    perror("accept");
    return -1;
  }

  return t;
}

int openconn(char *hostname, int port)
{	
  struct sockaddr_in sa;
  struct hostent *hp;
  int ret;
  
  if (!(hp = gethostbyname(hostname))) {
    perror("gethostbyname");
    return -1;
  }

  memset(&sa.sin_zero, 0, 8);
  sa.sin_port = htons(port);
  memcpy(&sa.sin_addr, hp->h_addr, hp->h_length);
  sa.sin_family = hp->h_addrtype;
  
  ret = socket(hp->h_addrtype, SOCK_STREAM, 0);
  if (connect(ret, (struct sockaddr *)&sa, sizeof(struct sockaddr_in))<0) {
    perror("connect");
    return -1;
  }

  return ret;
}
