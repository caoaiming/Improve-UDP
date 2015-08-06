#ifndef __THREADPOOL_H_
#define __THREADPOOL_H_

#include "udp.h"

#define DEFAULT_TIME  10 // every 10s check the task_queue and thread status
#define MIN_WAIT_TASK_NUM   10 // if queue_size > MIN_WAIT_TASK_NUM,
                               // we need add thread.

#define DEFAULT_THREAD_VARY 10 // #of  thread num vary.

#define ARG_SIZE            28 // arguments size.
typedef struct threadpool_t threadpool_t;

typedef struct Argument {
    int   pid;
    short port;
    char  ip[20];
}_Argument;


typedef struct {
    void *(*function)(void *);
    void *arg;
    bool status;
}threadpool_task_t;

typedef struct exit_thr_node{
    pthread_t id;
    struct exit_thr_node *next;
}exit_thr_node;

typedef struct {
    int cnt;
    exit_thr_node *tail;
    exit_thr_node *head;
}thread_join;


struct threadpool_t {
    pthread_mutex_t lock;              /* threadpool_t 结构体锁     */
    pthread_mutex_t thread_counter;    /* 线程数量锁                */
    pthread_cond_t queue_not_full;     /* 等待任务队列不满的情况发生*/
    pthread_cond_t queue_not_empty;    /* 等待任务队列不空的情况发生*/
    pthread_t *threads;               /* 线程队列                  */
    thread_join *queue_join;           /* 待回收队列                */
    pthread_t adjust_tid;              /* 管理线程                  */
    threadpool_task_t *task_queue;     /* 任务队列                  */
    int min_thr_num;                   /* 最少线程数量              */
    int max_thr_num;                   /* 最大线程数量              */
    int live_thr_num;                  /* 活跃线程数量              */
    int busy_thr_num;                  /* 在忙线程数量              */
    int wait_exit_thr_num;             /* 待回收的线程数量          */
    int queue_work;                    /* 任务队列已准备好的位置    */
    int queue_free;                    /* 队列队列未准备好的位置    */
    int queue_size;                    /* 队列当前待领取任务数量    */
    int queue_max_size;                /* 队列最大允许任务数量      */
    bool shutdown;                     /* 结束任务标志              */
};


threadpool_t *threadpool_create(int min_thr_num, int max_thr_num, 
        int queue_max_size);

int threadpool_add(threadpool_t *pool, void*(*function)(void *arg), 
        void *arg);

int threadpool_destroy(threadpool_t *pool);

int threadpool_all_threadnum(threadpool_t *pool);

int threadpool_busy_threadnum(threadpool_t *pool);
#endif// end __THREADPOOL_H_
