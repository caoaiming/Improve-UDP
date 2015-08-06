#include "udp.h"

#define ERRORLEN  1024

static void err_doit(int errnoflag, const char *fmt, va_list ap)
{
    int err_no, n;

    char buf[ERRORLEN + 1];

    err_no = errno;
    vsnprintf(buf, ERRORLEN, fmt, ap);

    n = strlen(buf);

    if(errnoflag)
        snprintf(buf+n, ERRORLEN - n, ": %s", strerror(err_no));
    strcat(buf, "\n");

    fflush(stdout);
    fputs(buf, stderr);
    fflush(stderr);

    return;
}

void err_sys(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(1, fmt, ap);
    va_end(ap);

    exit(1);
}

void err_quit(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(0, fmt, ap);
    va_end(ap);

    exit(1);
}

void err_msg(const char *fmt, ...)
{
    va_list ap;

    va_start(ap, fmt);
    err_doit(0, fmt, ap);
    va_end(ap);

    return;
}
