#include "udp.h"
#include "unpack.h"
#include "udp_rtt.h"
#include "recv.h"

#include <sys/epoll.h>
#include <sys/resource.h>

#define MAXEPOLLSIZE  10000

void display(struct pack_head *res)
{
    
        printf("总包数  ：  %d\n", res->pack_total);
        printf("类型    ：  %d\n", res->pack_type);
        printf("流水号  ：  %d\n", res->serial_no);
        printf("包序号  ：  %d\n", res->pack_seq);
        printf("偏移长度：  %d\n", res->pack_offset);
        printf("包大小  ：  %d\n", res->pack_size);
        printf("Checksum：  %u\n", res->Checksum);
        printf("ts      ：  %u\n", res->ts);
        printf("----------------------------------------\n");
        printf("buff : \n%s\n", res->buf);
        printf("----------------------------------------\n");
        printf("**************************************************\n\n");
}


int main(int ac, char **av)
{
    char buf[1024] = "Hello.\n";
    struct Ack ack;
    struct recv_ctl ctl;

    if(ac != 2)
        err_quit("Usage: udpcli<IPaddress>");

    bzero(&ack, ACK_SIZE);

    if (init_ctl(&ctl, 8888, av[1]) != OK)
        err_quit("init ctl error");

    sendto(ctl.sockfd, buf, strlen(buf), 0, (SA *)&ctl.addr, ctl.socklen);
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
                    return -1;
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

    return 0;
}
