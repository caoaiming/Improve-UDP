#include "udp.h"
#include "recv.h"
#include "unpack.h"

int init_ctl(struct recv_ctl *ctl, _Argument *arg)
{
    char pathbuf[PATH_SIZE] = {0};
    int  optval = 1;

    ctl->socklen = sizeof(struct sockaddr_in);
    bzero(&(ctl->addr), ctl->socklen);
    
    ctl->addr = arg->addr;
    ctl->sockfd = arg->sockfd;

    if ((arg->addr).sin_addr.s_addr == 0)
        ctl->con_flag = OFF;
    else
        ctl->con_flag = ON; //已经connect...
    
    setnonblocking(ctl->sockfd);

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

    ctl->pid = arg->pid;

#if 1
    ctl->key = get_ipc_key(ctl->pid);
    if (ctl->key == NG)
        return (NG);
    printf("ctl->key = %d\n", ctl->key);
#endif
    ctl->curfds = 1;
    ctl->overtime  = 0;
    ctl->size = 0;
    ctl->recv_flag = OFF;
    ctl->cnt = -1;
    
    bzero(&ctl->q_msg, sizeof(struct Queue_msg));
    bzero(&ctl->sureNO, sizeof(BUF_MAX_CNT * sizeof(int)));

    return (OK);
}

bool check_Allpack(int *no, struct Ack* ack, int totle)
{
    int i, index = 0;
    ack->ack_type = PT_Lost;

    for(i = 0; i < totle && index < 20; i++) {
        if (no[i] == 0) {
            (ack->seq)[index++] = i;
            //printf("重发丢失包编号 ： %d", i+1);
        }
    }
    if(index) {            //重发丢失包
        ack->ack_cnt = index;
        return NG;
    }
    //printf("接收完毕！\n");

    ack->ack_type = PT_Lastack; //全部接收
    return OK;
}

bool anwser_ack(struct Ack *ack, const struct pack_head *pack, 
                            int index, int able_buf, int *no, int *totle)
{
    if (pack->pack_type == PT_Data) {
        //printf("普通包...\n");
        ack->ack_type = PT_Acknowledge;
        (ack->seq)[index] = pack->pack_seq;
        ack->ts  = pack->ts;
        ack->ack_cnt = index + 1;
        ack->win = able_buf;
        *totle = pack->pack_total;
    }
    else if (pack->pack_type == PT_Fin) {
        //printf("结束包...\n");
        if (check_Allpack(no, ack, *totle) == OK) {
        //    printf("sucess.\n");
            return OK;
        }
    }
    return NG;
}

void getmsg_info(const struct pack_head *pack, char *buf, int *no)
{
    if (!no[pack->pack_seq - 1]) {
        no[pack->pack_seq - 1] = 1;
        memcpy(buf + pack->pack_offset, pack->buf, pack->pack_size);
    }
}
