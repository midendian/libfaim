/*
 * Common functions used in all apps
 */

int Read(int fd, unsigned char *buf, int len);
int establish(u_short portnum);
int get_connection(int s);
int openconn(char *hostname, int port);
