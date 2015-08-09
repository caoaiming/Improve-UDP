#include "udp.h"

int setnonblocking(int fd)
{
    ssize_t n;
    if ( fcntl(fd, F_SETFL, fcntl(fd, F_GETFD, 0) | O_NONBLOCK) < 0) {
        err_sys("set nonblcoking error.");
    }

    return (OK);
}

key_t get_ipc_key(pid_t pid)
{
    char pathName[PATH_LEN] = {0};
    key_t Key;

    sprintf(pathName, "%s%d", "/proc/", pid);
    Key = x_ftok(pathName, pid);

    if (Key == -1)
        return NG;

    return (Key);
}

int x_epoll_create(int size)
{
    int epll_fd;
    if ( (epll_fd = epoll_create(size)) < 0)
        err_sys("epoll_create error.");
    
    return (epll_fd);
}

int x_setrlimit(int resoure, const struct rlimit *rlim)
{
    int n;
    if ( (n = setrlimit(resoure, rlim)) < 0)
        err_sys("setrlimit error.");

    return (n);
}

int x_epoll_ctl(int epfd, int op, int fd, struct epoll_event *event)
{
    int n;
    if ( (n = epoll_ctl(epfd, op, fd, event)) < 0) {
        err_sys("epoll_ctl error.");
    }

    return (n);
}

int x_epoll_wait(int epfd, struct epoll_event *events, int maxevents,
        int timeout)
{
    int n;
    if ( (n = epoll_wait(epfd, events, maxevents, timeout)) < 0)
        err_sys("epoll_wait error.");
    
    return (n);
}

ssize_t x_recvmsg(int fd, struct msghdr *msg, int flags)
{
    ssize_t n;

    if ( (n = recvmsg(fd, msg, flags)) < 0 )
        err_sys("recvmsg error");
    return (n);
}

void x_sendmsg(int fd, const struct msghdr *msg, int flags)
{
    unsigned int  i;
    ssize_t  nbytes;

    nbytes = 0; /* must first figure out what return value should be */
    for (i = 0; i < msg->msg_iovlen; i++)
        nbytes += msg->msg_iov[i].iov_len;

    if(sendmsg(fd, msg, flags) != nbytes)
        err_sys("sendmsg error.");
}

void Gettimeofday(struct timeval *tv, void *foo)
{
    if (gettimeofday(tv, foo) == -1)
        err_sys("gettimeofday error.");
    return;
}

Sigfunc* Signal(int signo, Sigfunc *func)
{
    Sigfunc *sigfunc;

    if ( (sigfunc = signal(signo, func)) == SIG_ERR )
        err_sys("signal error.");

    return (sigfunc);
}

int sockfd_to_family(int sockfd)
{
    struct sockaddr_storage ss;

    socklen_t len;
    len = sizeof(ss);
    if(0 > getsockname(sockfd, (SA *)&ss, &len))
        return -1;
    return ss.ss_family;
}

/* include "x_socket"*/
int x_socket(int family, int type, int protocol)
{
    int n;

    if(0 > (n = socket(family, type, protocol)))
        err_sys("socket error");
    return n;
}
/*end "x_ocket"*/


void x_bind(int fd, const struct sockaddr *sa, socklen_t salen)
{
     if(0 > bind(fd, sa, salen)) {
          err_sys("bind error.");
      }
}

void x_connect(int fd, const struct sockaddr *sa, socklen_t salen)
{
    if(0 > connect(fd, sa, salen)) {
        err_sys("Connect error.");
    }
}

/*include Listen*/
void x_listen(int fd, int backlog)
{
    char *ptr;

    /*4can override 2nd argument with environment variable.*/
    if(NULL != (ptr = getenv("LISTENQ")))
        backlog = atoi(ptr);

    if(0 > listen(fd, backlog))
        err_sys("listen error");
}
/*end Listen*/

int x_setsockopt(int fd, int level, int optname, const void *optval, socklen_t len)
{
    int n;
    if ((n == (setsockopt(fd, level, optname, optval, len))) == -1)
        err_sys("setsockopt error.");

    return n;
}

int x_accept(int fd, struct sockaddr *sa, socklen_t *salenptr)
{
    int n;

again:
    if(0 > (n = accept(fd, sa, salenptr))) {
#ifdef EPROTO
        if(errno == EPROTO || errno == ECONNABORTED) /*Protocl error*/
#else
        if(errno = ECONNABORTED)    /*A connection has been aborted.*/
#endif
            goto again;
        else
            err_sys("accept error.");
    }
    return (n);
}

void x_close(int fd)
{
    if (-1 == close(fd))
        err_sys("close error.");
}

int x_fork(void)
{
    pid_t pid;

    if(-1 == (pid = fork()))
        err_sys("fork error.");
    return pid;
}

int x_select(int nfds, fd_set *readfds, fd_set *writefds, fd_set *exceptfds,
        struct timeval *timeout)
{
    int n;
    if( (n = select(nfds, readfds, writefds, exceptfds, timeout)) < 0 )
        err_sys("select error");

    return n;   /* can return 0 on timeout */
}

ssize_t x_recvfrom(int fd, void *ptr, size_t nbytes, int flags,
        struct sockaddr *sa, socklen_t *salenptr)
{
    ssize_t n;

    if( (n = recvfrom(fd, ptr, nbytes, flags, sa, salenptr)) < 0)
        err_sys("recvfrom error");

    return n;
}

void x_sendto(int fd, const void *ptr, size_t nbytes, int flags,
        const struct sockaddr *sa, socklen_t salen)
{
    if(sendto(fd, ptr, nbytes, flags, sa, salen) != (ssize_t)nbytes)
        err_sys("sendto error");

}

/*
 * Most are include in the source file for the function itself.
 */
const char * Inet_ntop(int family, const void *addrptr, char *strptr, size_t len)
{
    const char *ptr;

    if(strptr == NULL)   /* check for old code */
        err_sys("NULL 3rd argument to inet_ntop");
    if ((ptr = inet_ntop(family, addrptr, strptr, len)))
        err_sys("inet_ntop error");

    return ptr;
}

int Inet_aton(const char *cp, struct in_addr *inp)
{
    int n;
    if ( !(n = inet_aton(cp, inp)) )
        err_sys("inet_aton error.");
    
    return n;
}

void Inet_pton(int family, const char *strptr, void *addrptr)
{
    int   n;

    if (0 > (n = inet_pton(family, strptr, addrptr)))
        err_sys("inet_pton error for %s", strptr); /* errno set*/
    else if (n == 0)
        err_sys("inet_pton error for %s", strptr); /*errno not set*/

    /*nothing to return*/
}

 /* write "n" bytes to a descriptor. */
ssize_t write_n(int fd, const void *vprt, size_t n)
{
    size_t  n_left;
    ssize_t n_written;
    const char *ptr;

    ptr = vprt;
    n_left = n;

    while(n_left > 0) {
        if((n_written = write(fd, ptr, n_left)) <= 0) {
            if(n_written < 0 && errno == EINTR)
                n_written = 0;             /* and call write() again */
/*The  call  was interrupted by a signal before any data was writ‐ten*/         
            else
                return (-1);               /* error. */
        }

        n_left -= n_written;
        ptr    += n_written;
    }
    return (n);
}
/*end write_n*/

void x_write(int fd, void *ptr, size_t nbytes)
{
    if(write_n(fd, ptr, nbytes) != nbytes)
        err_sys("write_n error");
}

int x_mkfifo(const char *pathname, mode_t mode)
{
    int n;
    if (mkfifo(pathname, mode) < 0 && (errno != EEXIST))
        err_sys("can't create mkfifo ");

    return (n);
}

int x_open(const char *pathname, int flags, mode_t mode)
{
    int n;

    if ((n = open(pathname, flags, mode)) < 0)
        err_sys("open fifo error");

    return (n);
}

void * x_malloc(size_t size)
{
    void *ptr;

    if (size == 0) {
        err_msg("malloc: zero size.");
    }

    ptr = (void *)malloc(size);

    if (ptr == NULL)
        err_quit("malloc: out of memory (allocating %lu bytes).\n", size);

    return ptr;
}

void *x_calloc(size_t nmemb, size_t size)
{
    void *ptr;

    if (size == 0|| nmemb == 0) {
        err_msg("calloc: zero size.");
    }

    ptr = (void *)calloc(nmemb, size);

    if (ptr == NULL) {
        err_quit("calloc: out of memory (allocating %lu bytes).",size*nmemb);
    }

    return ptr;
}

void* x_realloc(void *oldptr, size_t nmemb, size_t size)                  
{
    void *newptr;
    size_t new_size = nmemb * size;

    if (new_size == 0) {
        err_msg("realloc: zero size");
        return oldptr;
    }

    if (oldptr == NULL)
        newptr = malloc(new_size);
    else
        newptr = realloc(oldptr, new_size);
    
    if (newptr == NULL) {
        err_quit("realloc: out of memory (allocating %lu bytes).", 
                                                                 new_size);
    }

    return newptr;
}

void x_free(void *ptr)
{
    if (ptr == NULL)
        err_msg("NULL pointer is given as argument.");
    else
        free(ptr);
}

key_t x_ftok(const char *pathname, int proj_id)
{
    key_t key;

    if ( (key = ftok(pathname, proj_id)) == -1)
        err_sys("ftok error.");

    return (key);
}

int x_msgget(key_t key, int msgflg)
{
    int n;

    if ( (n = msgget(key, msgflg)) == -1)
        err_msg("msgget error.");

    return (n);
}

int x_msgctl(int msqid, int cmd, struct msqid_ds *buf)
{
    int n;

    if ( (n = msgctl(msqid, cmd, buf)) == -1)
        err_msg("msgctl error.");

    return (n);
}

int x_msgsnd(int msqid, const void *msgp, size_t msgsz, int msgflg)
{
    int n;

    if ( (n = msgsnd(msqid, msgp, msgsz, msgflg)) == -1) 
            err_msg("msgsnd error.");
    
    return (n);
}

ssize_t x_msgrcv(int msqid, void *msgp, size_t msgsz, long msgtyp, int msgflg)
{
    ssize_t n;

    if( (n = msgrcv(msqid, msgp, msgsz, msgtyp, msgflg)) == -1)
        err_msg("msgrcv error.");

    return (n);
}
