#include "udp_rtt.h"
#include "unpack.h"

#include <setjmp.h>

#define RTT_DEBUG


static struct rtt_info rttinfo;
static int rttinit = 0;

static void sig_alrm(int signo);
static sigjmp_buf jmpbuf;

static void sig_alrm(int signo0)
{
    siglongjmp(jmpbuf, 1);
}

static bool make_sure(struct Ack *ackmsg, _pack *data)
{
    int i;
    for (i = 0; i < ackmsg->ack_cnt; i++)
        data[(ackmsg->seq)[i]].flag = OK;
    
    return (OK);
}
static void init_sliding_window(struct sliding_win *window)
{
    window->cnwd = WIN_SLOW_START;   // cnwd = 1
    window->ssthresh = SSTH_DEFAULT_SIZE;
    window->win_size =  1; //start in sliding mathod...
    window->win_start = 0;
    window->win_end = 0;
    window->step_add = 1;

    return;
}

static int fflush_window(struct sliding_win *window, int win_recvsize)
{
    int win;
    if (window->cnwd < SSTH_DEFAULT_SIZE / 2) //处于慢启动阶段
        window->step_add =  (window->step_add) * 2;
    else {
        window->ssthresh = SSTH_DEFAULT_SIZE; //处于拥塞控制阶段
        window->step_add = 1;
    }

    window->cnwd += window->step_add;
    win = min(window->cnwd, win_recvsize);

    return (win);
}

ssize_t rtt_udp_send(int fd, const void *outbuff, size_t outbytes, 
        struct sockaddr *dest, socklen_t addrlen)
{
    ssize_t n;
    _pack *data;
    struct Ack ackmsg;
    int i = 0, j = 0, losti = 0;
    struct sockaddr addr;
    int cnt;
    int lose_flag = OFF;
    int finish_flag = OFF;
    int Error;

    struct pack_head Finish;   /*发送完毕*/
    Finish.pack_type = PT_Fin;

    struct sliding_win window;
    init_sliding_window(&window); /*init sliding window.*/
                                  /* split packet */ 
    data = (_pack *)(UL)split_pack(outbuff, outbytes, &cnt); 
    //printf("cnt = %d\n", cnt);

    if (rttinit == 0) {
        rtt_init(&rttinfo);  /* first time we're called */
        rttinit = 1;
        rtt_d_flag = 1;
    }

    Signal(SIGALRM, sig_alrm);
    rtt_newpack(&rttinfo);  /* initialize for this packet */

    for (i = 0; i < cnt; i = window.win_end) {
        window.win_start = i;
        window.win_end = min(window.win_start + window.win_size, cnt);
        printf("window = %d\n", window.win_size);
sendagain:
        for (j = window.win_start; j < window.win_end && j < cnt; j++) {
            data[j].pack.ts = rtt_ts(&rttinfo);
            Error = sendto(fd,&data[j].pack,INTER_NET_SIZE,0,dest,addrlen);
            if (Error == -1) { //终止发送,报错返回..
                delete(data, cnt);
                return NG;
            }//end if
        }//end for

        if(j == cnt)
            finish_flag = ON;
        
calctime:
        alarm(rtt_start(&rttinfo)); /* calc timeout value & start timer */

        if (sigsetjmp(jmpbuf, 1) != 0) {
            if (rtt_timeout(&rttinfo) < 0) {
                err_msg("dg_send_recv: no response from server, giving up");
                rttinit = 0; /*reinit int case we're called again */
                errno = ETIMEDOUT;
                return (NG);
            }
            window.cnwd = 1;            //慢启动

            if (lose_flag == ON) {
                goto sendlostagain;
            }//end if
            else if (finish_flag == ON) {
                goto sendfinagain;
            }//end else-if
            else {
                goto sendagain;
            }//end else
        }//end if
                                     /*waitting ack ... */
        do 
        {
            n = recvfrom(fd, &ackmsg, ACK_SIZE, 0, &addr, &addrlen);
        } while (n != ACK_SIZE);
        alarm(0);                  /* stop SIGALRM timer */
        
        lose_flag = OFF;

        switch (ackmsg.ack_type) {
            case  PT_Lastack :    /* finish send, over! */
                //printf("Finish.....\n");
                goto finish;
                break;
        
            case  PT_Acknowledge :
                window.win_size = fflush_window(&window, ackmsg.win);
                make_sure(&ackmsg, data);
                break;

            case  PT_Lost :
                lose_flag == ON;
sendlostagain:
                for (losti = 0; losti < ackmsg.ack_cnt; losti++) {
                    data[ackmsg.seq[losti]].pack.ts = rtt_ts(&rttinfo);
                    sendto(fd, &data[ackmsg.seq[losti]].pack, 
                                    INTER_NET_SIZE,  0, dest, addrlen);
                    //printf("%d send\n", losti+1);
                }
                goto calctime;
                break;
                
            default :
                break;
        }
                          /*calculate &store new RTT estimator values */
        rtt_stop(&rttinfo, rtt_ts(&rttinfo) - ackmsg.ts);
        if (finish_flag == ON) {  /*结束标志包*/
sendfinagain:
            Finish.ts = rtt_ts(&rttinfo);
            Error = sendto(fd, &Finish, INTER_NET_SIZE, 0, dest, addrlen);
            if (Error == -1) {   /* 终止发送,报错返回.. */
                delete(data, cnt);
                return NG;
            }//end if
            //printf("发送了Fin\n");
            goto calctime;
        }

    }//end for

finish:
    delete(data, cnt);//释放数据
    return OK;
}
