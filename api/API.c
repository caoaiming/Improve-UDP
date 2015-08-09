#include "recv.h"
#include "udp.h"

int sock_init(char *ip, int port, struct sockaddr_in *addr)
{
    int sockfd = -1;
    int optval = 1;

    if ( port < 1024)
        return NG;
    bzero(addr, sizeof(struct sockaddr));

    sockfd = x_socket(AF_INET, SOCK_DGRAM, 0);
    addr->sin_family = AF_INET;
    addr->sin_port = htons(port);
    
    if (ip == NULL) {
        addr->sin_addr.s_addr = htonl(INADDR_ANY);
        x_bind(sockfd, (SA *)addr, sizeof(struct sockaddr));
        printf("bind OK\n");
    }
    else  {
        Inet_pton(AF_INET, ip, &(addr->sin_addr));
        x_connect(sockfd, (SA *)addr, sizeof(struct sockaddr));
    }

    x_setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &optval, sizeof(int));


    return sockfd;
}

int start_udp_recv(int fd, struct sockaddr_in *addr ,int *id)
{
    _Argument argv;
    bzero(&argv, sizeof(_Argument));
    int writfd;
    pid_t  pid;
    key_t  key;
    int qmsg_id;
    ssize_t n;
    int try_cnt = 2;

    *id = -1;
    pid = getpid();

    argv.pid = pid;
    argv.addr = *addr;
    argv.sockfd = fd;

    if ( (writfd = open(SERV_FIFO, O_WRONLY, 0)) < 0)
        return NG;

    if ( (n = write(writfd, &argv, sizeof(_Argument)) < 0)) //提交接收请求
        return NG;

    if  ( (key = get_ipc_key(pid)) == NG)
        return NG;
    printf("ipc_key create is %d\n", key);

    sleep(1);
try_again:
    qmsg_id = msgget(key, IPC_EXCL);

    if ( qmsg_id < 0) {
        if (EEXIST == errno) {
            printf("This key is exist.\n");
            qmsg_id = msgget(key, 0); //连接消息队列
            printf("消息队列： ipc_id = %d\n", qmsg_id);
            if (qmsg_id < 0)
                return NG;
        }
        else {
            if (--try_cnt) {
                sleep(1); //等待udp服务端启动线程
                goto try_again;
            } else {
                return NG;
            }
        }
    }
    *id = qmsg_id;

    return OK;
}

int udp_recv(int msqid, void *msgpi, struct sockaddr_in *sock_addr, int sta)
{
    struct Queue_msg Qmsg;
    ssize_t n;

    bzero(&Qmsg, sizeof(struct Queue_msg));

    if (msgpi == NULL)
        return NG;

    n = msgrcv(msqid, &Qmsg, sizeof(struct Queue_msg), 0, 0);
    if (n < 0)
        return NG;
    if (sock_addr != NULL)
        *sock_addr = Qmsg.addr;
    memcpy(msgpi, Qmsg.buf, BUF_MAX_SIZE);

    return (sizeof(Qmsg.buf));

}

ssize_t udp_send(int fd, const void *outbuf, size_t outsize, 
                    struct sockaddr_in *dest, socklen_t addrlen)
{
    ssize_t n;

    n = rtt_udp_send(fd, outbuf, outsize, dest, addrlen);

    return (n);
}
