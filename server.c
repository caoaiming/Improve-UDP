#include "udp_rtt.h"
#include "unpack.h"

#include <setjmp.h>

#define RTT_DEBUG

/*
#define SSTH_DEFAULT_SIZE       20
#define WIN_INIT_SIZE           1
#define WIN_MIN_SIZE            2
*/

static struct rtt_info rttinfo;
static int rttinit = 0;

static void sig_alrm(int signo);
static sigjmp_buf jmpbuf;

static void sig_alrm(int signo0)
{
    siglongjmp(jmpbuf, 1);
}

bool make_sure(struct Ack *ackmsg, _pack *data)
{
    int i;
    for (i = 0; i < ackmsg->ack_cnt; i++)
        data[(ackmsg->seq)[i]].flag = OK;
    
    return (OK);
}
void init_sliding_window(struct sliding_win *window)
{
    window->cnwd = WIN_SLOW_START;   // cnwd = 1
    window->ssthresh = SSTH_DEFAULT_SIZE;
    window->win_size =  1; //start in sliding mathod...
    window->win_start = 0;
    window->win_end = 0;
    window->step_add = 1;

    return;
}

int fflush_window(struct sliding_win *window, int win_recvsize)
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

ssize_t udp_send(int fd, const void *outbuff, size_t outbytes, 
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

    struct pack_head Finish;   /*发送完毕*/
    Finish.pack_type = PT_Fin;

    struct sliding_win window;
    init_sliding_window(&window); /*init sliding window.*/
    
    data = (_pack *)split_pack(outbuff, outbytes, &cnt); /* split packet */
    printf("cnt = %d\n", cnt);

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
            sendto(fd, &data[j].pack, INTER_NET_SIZE, 0, dest, addrlen);
        }
        if(j == cnt)
            finish_flag = ON;
        
calctime:
        alarm(rtt_start(&rttinfo)); /* calc timeout value & start timer */

        if (sigsetjmp(jmpbuf, 1) != 0) {
            if (rtt_timeout(&rttinfo) < 0) {
                err_msg("dg_send_recv: no response from server, giving up");
                rttinit = 0; /*reinit int case we're called again */
                errno = ETIMEDOUT;
                return (-1);
            }
            window.cnwd = 1;            //慢启动
            if(lose_flag == ON)
                goto sendlostagain;
            else if (finish_flag == ON)
                goto sendfinagain;
            else
                goto sendagain;
        }

                                  /*waitting ack ... */
        do {
                n = recvfrom(fd, &ackmsg, ACK_SIZE, 0, &addr, &addrlen);
        } while (n != ACK_SIZE);
        alarm(0);                /* stop SIGALRM timer */
        
        lose_flag = OFF;

        switch (ackmsg.ack_type) {
            case  PT_Lastack :   /* finish send, over! */

                printf("Finish.....\n");
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
                    printf("%d send\n", losti+1);
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
            sendto(fd, &Finish, INTER_NET_SIZE, 0, dest, addrlen);
            printf("发送了Fin\n");
            goto calctime;
        }

    }//end for

finish:
    return OK;
}


int main(int ac, char **av)
{
    int sockfd;
    struct sockaddr_in addr;
    socklen_t socklen = sizeof(struct sockaddr_in);
    int i;

    struct sockaddr_in client;
    char info[65535] = {0};
    int recvsize = 0;

    sockfd = sock_init(NULL, &addr);
    recvsize = recvfrom(sockfd, &info, 65535, 0, (SA *)&client, &socklen);

    printf("%s is joind />: ...\n", inet_ntoa(client.sin_addr));
    printf("%s\n\n\n", info);

    int fd = open("123.txt", O_RDONLY);
    bzero(info, 65535);


    ssize_t n = 0;
    //if((n = read(fd,info, 1023)) == 1023)
    //for(i = 0; i < 3; i++) {
         n = read(fd,info, 65534);
         printf("发这么多... %d, \n",n);
         udp_send(sockfd, info, 65534, (SA *)&client, socklen);
    //}
        //TODO..
    close(fd);
    
    return 0;
}
