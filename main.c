#include "udp.h"
#include "unpack.h"
#include "threadpool.h"
#include "recv.h"
#include "udp_rtt.h"

#include <sys/epoll.h>
#include <sys/resource.h>

#define MAXEPOLLSIZE  10000

void *work(void *arg)
{
    char buf[1024];
    struct Ack ack;
    struct recv_ctl ctl;
    _Argument *argv = (_Argument *)arg;

    if(arg == NULL)
        err_quit("Usage: udpcli<IPaddress>");

    bzero(&ack, ACK_SIZE);


    if (init_ctl(&ctl, argv->port, argv->ip) != OK)
        err_quit("init ctl error");

    //sendto(ctl.sockfd, buf, strlen(buf), 0, (SA *)&ctl.addr, ctl.socklen);
    for (;;) {
        bzero(buf, 1024);
golisten:
        ctl.nfds = x_epoll_wait(ctl.epoll_fd, ctl.events, 
                ctl.curfds,ctl.overtime);
        if (ctl.nfds == 0 && ctl.recv_flag == OFF) {
            ctl.overtime = RECV_NO;
            continue;
        } else if (ctl.nfds) {
            if (ctl.events[0].data.fd == ctl.sockfd) {
                ctl.overtime = RECV_YES;
                ctl.cnt++;
                ctl.recv_flag = ON;

                if ( (ctl.size = read(ctl.sockfd, buf, 1024)) < 0) {
                    printf("[%d]对方断开....\n", errno);
                    return NULL;
                }

                if(((struct pack_head *)buf)->pack_type == PT_Data)
                    getmsg_info(buf, ctl.recvbuff, ctl.sureNO);

                if (anwser_ack(&ack, buf, ctl.cnt, 10,
                            &ctl.sureNO, &ctl.total) == OK) {
                    ctl.recv_flag = FINISH;
                    goto gosend;
                }
                goto golisten;
            }//end if
        }// end else-if
gosend:
        printf("收到组中第 %d 个\n", ctl.cnt+1);
        //sendto(sockfd, &ack, ACK_SIZE, 0, (SA *)&servaddr, socklen);
        sendto(ctl.sockfd, &ack, ACK_SIZE, 0, NULL, ctl.socklen);
        if(ctl.recv_flag == FINISH)
            //提交消息队列，清空缓存...
        ctl.recv_flag = OFF;
        ctl.cnt = -1;
    }//over for
    //printf("size = %d \n%s$\n\n\n", strlen(ctl.recvbuff) ,ctl.recvbuff);
    close(ctl.epoll_fd);
    close(ctl.sockfd);

    return NULL;
}


int main(int ac, char **av)
{
    int n;
    int readfd;
    _Argument arg;
    threadpool_t *thp = threadpool_create(2, 100, 15);
    char buf[1024] = {0};

    //printf("pool inited./>: \n");

    mkfifo(SERV_FIFO, FILE_MODE);
    readfd = open(SERV_FIFO, O_RDONLY, 0);
    
    bzero(&arg, sizeof(_Argument));


    for (;;) {
        bzero(buf, 1024);
        if((n = read(readfd, buf, ARG_SIZE)) > 0)
            threadpool_add(thp, work, (void *)buf);
        else
            break;
    }

    threadpool_destroy(thp);
    close(readfd);
    unlink(SERV_FIFO);
    free(thp);

    return 0;
}
