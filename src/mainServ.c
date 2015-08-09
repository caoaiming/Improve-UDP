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
    char buf[BUF_RECV_SIZE];  // recved data < 1500
    struct Ack ack;
    struct recv_ctl ctl;
    _Argument *argv = (_Argument *)arg;
    int msg_len = sizeof(struct Queue_msg);

    bzero(&ack, ACK_SIZE);

    if (init_ctl(&ctl, arg) != OK)
        err_quit("init ctl error");

    if ( (ctl.msqid = x_msgget(ctl.key, SVMSG_MODE | IPC_CREAT) == -1))
        err_sys("stop work.");
    /* 由线程创建的消息队列返回的id值为0, 需要重新直接获取... */
    ctl.msqid = msgget(ctl.key, 0);
    ctl.q_msg.type = 1;

    for (;;) {
        bzero(buf, BUF_RECV_SIZE);
golisten:
        ctl.nfds = x_epoll_wait(ctl.epoll_fd, ctl.events, 
                ctl.curfds,ctl.overtime);

        if (ctl.nfds == 0 && ctl.recv_flag == OFF) {
            //printf("无数据到来。。。\n");
            ctl.overtime = RECV_NO;
            continue;
        } 
        else if (ctl.nfds) {
            if (ctl.events[0].data.fd == ctl.sockfd) {
                //printf("%d\n有数据到来。。。\n", ctl.con_flag);
                ctl.overtime = RECV_YES;
                ctl.cnt++;
                ctl.recv_flag = ON;

                if (ctl.con_flag == ON) {//有connect属性,作为UDP客户端程序
                    ctl.size = read(ctl.sockfd, buf, BUF_RECV_SIZE);
                }
                else             {       //作为UDP服务器监听任何连接的到来
                    ctl.size = recvfrom(ctl.sockfd, buf, BUF_RECV_SIZE, 0, 
                            (SA *)&ctl.q_msg.addr, &ctl.socklen);
                    if (ctl.size == -1) //sockfd失效,客户退出
                        break; //终止循环
                    
                }

                if (ctl.size  < 0) {
                    if (ctl.con_flag == ON) {
                        //printf("[%d]对方断开....\n", errno);
                        memset(buf, 0x00, BUF_RECV_SIZE); /* drop data */
                        sprintf(buf, "%c", '#'); 
                                            /* '#' is means stop connect */
                        x_msgsnd(ctl.msqid, &buf, 1, 0);
                        break;              /* over receive and return */
                    }
                    else
                        goto golisten;
                }

                if(((struct pack_head *)buf)->pack_type == PT_Data)
                    getmsg_info(buf, ctl.q_msg.buf, ctl.sureNO);

                if (anwser_ack(&ack, buf, ctl.cnt, WIN_DEFAULT_SIZE,
                            &ctl.sureNO, &ctl.total) == OK) {
                    ctl.recv_flag = FINISH;
                    goto gosend;
                }
                goto golisten;
            }//end if
        }// end else-if
gosend:
        //printf("收到组中第 %d 个\n", ctl.cnt+1);
        if (ctl.con_flag == OFF) { //没有connect属性
            sendto(ctl.sockfd, &ack, ACK_SIZE, 0, (SA *)&ctl.q_msg.addr, ctl.socklen);
        }
        else
            sendto(ctl.sockfd, &ack, ACK_SIZE, 0, NULL, ctl.socklen);
        
        if(ctl.recv_flag == FINISH) {
            x_msgsnd(ctl.msqid, &ctl.q_msg, msg_len, IPC_NOWAIT);
                            //提交给消息队列,不考虑消息或字节太多溢出...
        }
        ctl.recv_flag = OFF;
        ctl.cnt = -1;
    }//over for
    x_msgctl(ctl.msqid, IPC_RMID, NULL);
    close(ctl.epoll_fd);
    close(ctl.sockfd);

    return NULL;
}


int main(int ac, char **av)
{
    int n;
    int readfd;
    _Argument arg;                                  //模拟默认值
    threadpool_t *thp = threadpool_create(3, 100, 15);
    char buf[BUF_RECV_SIZE] = {0};

    daemon_init(av[1], 0);   /* become daemon process */

    mkfifo(SERV_FIFO, FILE_MODE);
    readfd = open(SERV_FIFO, O_RDONLY, 0);
    
    bzero(&arg, sizeof(_Argument));


    for (;;) {
        bzero(buf, 1024);
        if((n = read(readfd, &arg, ARG_SIZE)) > 0) {
            //printf("%s is joind />: ...\n",inet_ntoa(arg.addr.sin_addr));
            //printf("%d\n", arg.addr.sin_port);
            threadpool_add(thp, work, (void *)&arg);
            bzero(&arg, sizeof(_Argument));
        }
        else
            break;
    }

    threadpool_destroy(thp); //销毁线程池
    close(readfd);
    unlink(SERV_FIFO);
    free(thp);

    return 0;
}

