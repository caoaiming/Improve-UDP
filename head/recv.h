#ifndef __RECV_H_
#define __RECV_H_

#define BUF_MAX_SIZE     4096
#define BUF_MAX_CNT      128
#define BUF_RECV_SIZE    1500
#define PATH_SIZE        64

#define RECV_YES         5
#define RECV_NO          10        

#include "udp.h"

typedef struct Argument {       /*thread arguments */
    int    pid;
    int    sockfd;
    struct sockaddr_in addr;
}_Argument;

#if 0
struct Queue_data
{
    struct sockaddr_in addr;
    char   buf[BUF_MAX_SIZE];
};
#endif

struct Queue_msg {
    long   type;
    struct sockaddr_in addr;
    char   buf[BUF_MAX_SIZE];
};


struct recv_ctl {
    int sockfd;
    int epoll_fd;
    int curfds;
    int nfds;
    int size;
    int recv_flag;
    int cnt;
    int con_flag;
    int overtime;
    int total;
    int serial;
    pid_t pid;
    key_t key;
    int msqid;
    socklen_t socklen;
    struct sockaddr_in addr;
    struct epoll_event ev;
    struct epoll_event events[1];
    struct Queue_msg q_msg;
    int    sureNO[BUF_MAX_CNT];
};

#endif //end _recv_h_
