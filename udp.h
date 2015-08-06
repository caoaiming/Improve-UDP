#ifndef _UDP_H_
#define _UDP_H_

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include <stdarg.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <fcntl.h>

#include <time.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <signal.h>
#include <errno.h>

#include <pthread.h>
#include <sys/resource.h>
#include <sys/epoll.h>

typedef unsigned int  bool; 

typedef void Sigfunc(int);  /* for signal handlers */    
#define  SA  struct sockaddr

#define  OK         1  /* success */
#define  NG        -1  /* faliure */

#define  TRUE       1  /* true  */
#define  FALSE      0  /* false */

#define  OFF        0
#define  ON         1
#define  FINISH     2

#define  IP_LENGTH  20

#define min(a,b)   ((a) < (b) ? (a) : (b))
#define max(a,b)   ((a) > (b) ? (a) : (b))

#define SERV_FIFO   "/tmp/fifo.1"
#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)


#endif //end udp_h
