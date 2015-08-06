#ifndef __RECV_H_
#define __RECV_H_

#define BUF_MAX_SIZE     65535
#define BUF_MAX_CNT      128

#define RECV_YES         5
#define RECV_NO          10        


struct recv_ctl {
    int sockfd;
    int epoll_fd;
    int curfds;
    int nfds;
    int size;
    int recv_flag;
    int cnt;
    int overtime;
    int total;
    int serial;
    socklen_t socklen;
    struct sockaddr_in addr;
    struct epoll_event ev;
    struct epoll_event events[1];
    char   recvbuff[BUF_MAX_SIZE];
    int    sureNO[BUF_MAX_CNT];
};

#endif //end _recv_h_
