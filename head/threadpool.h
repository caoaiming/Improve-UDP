#ifndef __THREADPOOL_H_
#define __THREADPOOL_H_

#include "udp.h"
#include "list.h"
#include "recv.h"

#define DEFAULT_TIME  10 //every 10s check the task_queue and thread status

/* if queue_size > MIN_WAIT_TASK_NUM, we need add thread. */
#define MIN_WAIT_TASK_NUM   10

#define DEFAULT_THREAD_VARY 10   /* #of  thread num vary. */

#define ARG_SIZE            28   /* arguments size. */
typedef struct threadpool_t threadpool_t;


typedef struct {                /* task structure */
    void *(*function)(void *);
    _Argument arg;
    bool status;
}threadpool_task_t;


struct join_queue {
    struct list_head join_list_node;  /* 链表节点 */
    pthread_t        join_tid;        /* 线程id   */
};

struct threadpool_t {
    pthread_mutex_t lock;              /* threadpool_t 结构体锁     */
    pthread_mutex_t thread_counter;    /* 线程数量锁                */
    pthread_cond_t queue_not_full;     /* 等待任务队列不满的情况发生*/
    pthread_cond_t queue_not_empty;    /* 等待任务队列不空的情况发生*/
    pthread_t *threads;                /* 线程队列                  */
    pthread_t adjust_tid;              /* 管理线程                  */
    threadpool_task_t *task_queue;     /* 任务队列                  */
    struct list_head  *join_list;      /* 待回收队列                */
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

static void *threadpool_thread(void *threadpool);
static void *adjust_thread(void *threadpool);
static bool is_thread_alive(pthread_t tid);
static void threadpool_free(threadpool_t *pool);
static int  find_free_task(threadpool_t *pool);
static int  find_work_task(threadpool_t *pool);
static void put_claen_queue(threadpool_t *pool, pthread_t id);
static void recover_clean_queue(threadpool_t *pool);
static int  find_thread_index(threadpool_t *pool, pthread_t id);
static void destroy_list(threadpool_t *pool);

threadpool_t *threadpool_create(int min_thr_num, int max_thr_num, 
                                                      int queue_max_size);

int threadpool_add(threadpool_t *pool, void*(*function)(void *arg), 
                                                               void *arg);

int threadpool_destroy(threadpool_t *pool);
static int threadpool_all_threadnum(threadpool_t *pool);
static int threadpool_busy_threadnum(threadpool_t *pool);

#endif// end __THREADPOOL_H_
