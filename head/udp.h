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

#include <sys/ipc.h>
#include <sys/msg.h>
typedef unsigned char  bool; 

typedef void Sigfunc(int);  /* for signal handlers */    

#define  SA  struct sockaddr
#define  UL  unsigned long


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

#define PATH_LEN    128

#define MSG_R   0400
#define MSG_W   0200

#define FILE_MODE   (S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH)
                   /* default permissions for new files */
#define DIR_MODE    (FILE_MODE | S_IXUSR | S_IXGRP | S_IXOTH)
                   /* default permissions for new directories */
#define SVMSG_MODE  (MSG_R | MSG_W | MSG_R>>3 | MSG_R>>6)
                   /* default permissions for new SystemV message */
#define SVSEM_MODE  (SEM_R | SEM_A | SEM_R>>3 | SEM_R>>6)
                   /* default permissions for new SystemV semaphores */
#define SVSHM_MODE  (SHM_R | SHM_W | SHM_R>>3 | SHM_R>>6)
                   /* default permissions for new SystemV shared memory */


#endif //end udp_h
