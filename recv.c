#include "udp.h"
#include "recv.h"
#include "error.h"

int init_ctl(struct recv_ctl *ctl, short port, char *ip)
{
                                         /* init sockfd */
    ctl->socklen = sizeof(struct sockaddr_in);

    if(ip == NULL || !port)
        err_quit("Usage: need <IP address>  <port>");
    bzero(&(ctl->addr), ctl->socklen);

    ctl->addr.sin_family = AF_INET;
    ctl->addr.sin_port = htons(port);
    Inet_pton(AF_INET, ip, &ctl->addr.sin_addr);

    ctl->sockfd = x_socket(AF_INET, SOCK_DGRAM, 0);
    setnonblocking(ctl->sockfd);
    x_connect(ctl->sockfd, (SA *)&ctl->addr, ctl->socklen);

    /*init epoll*/
    ctl->epoll_fd = x_epoll_create(1);     /* only for sockfd */
    ctl->ev.events = EPOLLIN;              /* case for input  */
    ctl->ev.data.fd = ctl->sockfd;
    x_epoll_ctl(ctl->epoll_fd, EPOLL_CTL_ADD, ctl->sockfd, &ctl->ev);

#if 0
    struct rlimit rt;
    rt.rlim_max = rt.rlim_cur = MAXEPOLLSIZE;
    x_setrlimit(RLIMIT_NOFILE, &rt);
#endif 

    ctl->curfds = 1;
    ctl->overtime  = 0;
    ctl->size = 0;
    ctl->recv_flag = OFF;
    ctl->cnt = -1;
    bzero(ctl->recvbuff, BUF_MAX_SIZE);
    bzero(ctl->sureNO, BUF_MAX_CNT);

    return (OK);
}
