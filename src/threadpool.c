#include "threadpool.h"
#include "recv.h"


//创建线程池
threadpool_t *threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size)
{
    threadpool_t *pool = NULL;

    do {
        if ((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL){
            debug1_msg("malloc threadpool fail");
            break;
        }
        
        pool->min_thr_num = min_thr_num;
        pool->max_thr_num = max_thr_num;
        pool->busy_thr_num = 0;            //在工作当中的线程
        pool->live_thr_num = min_thr_num;  //处于活动当中的线程数量
        pool->queue_size = 0;              //任务数量
        pool->queue_max_size = queue_max_size;
        pool->queue_work = 0;             //已准备好任务位置
        pool->queue_free = 0;              //未准备好任务位置
        pool->shutdown = FALSE;            //终止服务的标志

        pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * max_thr_num);
        if (pool->threads == NULL) {
            debug1_msg("malloc pool threads error.");
            break;
        }

        memset(pool->threads, 0, sizeof(pool->threads));

        LIST_HEAD_P(nf_jointid);          //申请一个链表头指针
        INIT_LIST_HEAD(nf_jointid);
        pool->join_list = nf_jointid;
        

        //创建任务队列
        pool->task_queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t)*queue_max_size);
        if (pool->task_queue == NULL) {
            debug1_msg("malloc task queue fail.");
            break;
        }
        
        //初始化任务队列...
        int i;
        for (i = 0; i < queue_max_size; i++) {
            bzero(&(pool->task_queue[i].arg), sizeof(_Argument));
            pool->task_queue[i].status = OFF; //no use
        }
        //初始化锁与条件变量
        if (pthread_mutex_init(&(pool->lock), NULL) != 0
        || pthread_mutex_init(&(pool->thread_counter), NULL) != 0
        || pthread_cond_init(&(pool->queue_not_empty), NULL) != 0
        || pthread_cond_init(&(pool->queue_not_full), NULL ) != 0)
        {
            printf("init the lock or cond fail.\n");
            break;
        }
        //启动任务线程
        for (i = 0; i < min_thr_num; i++)
        {
            pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
            printf("start thread %d...\n", pool->threads[i]);
        }
        //启动管理线程
        pthread_create(&(pool->adjust_tid), NULL, adjust_thread, (void *)pool);

        return pool; //That's OK.
    }while(0);

    threadpool_free(pool);
    return NULL;
}

static int find_thread_index(threadpool_t *pool, pthread_t id)
{
    int i;
    for (i = 0; i < pool->max_thr_num && id != pool->threads[i]; i++)
        ;
    return i;
}

//放入回收链表
static void put_clean_queue(threadpool_t *pool, pthread_t id)
{

    struct join_queue *new = (struct join_queue *)malloc(sizeof(struct join_queue));

    new->join_tid   = id;

    list_add_tail(&(new->join_list_node), pool->join_list);
}

static void recover_clean_queue(threadpool_t *pool)
{
    struct list_head *i;

    if ( !list_empty(pool->join_list)) {         //其实没必要
        list_del_for_each (i, pool->join_list) {
            struct join_queue *jops = (struct join_queue *)i;
            
            if (pthread_join(jops->join_tid, NULL))
                debug1_msg("%d, 线程回收失败.", jops->join_tid);
            
            //printf("线程回收成功...\n");
            
            list_del(i);        /* 从链表孤立节点 */
            free(jops);         /* 释放用户数据   */
        }// end list_del_for_each
    }//end if
}

static void destroy_list(threadpool_t *pool)
{
    struct list_head *i;

    if (pool->join_list) {
        list_del_for_each (i, pool->join_list) {
            struct join_queue *jops = (struct join_queue *)i;

            list_del(i);        
            free(jops);
        }//end list_del_for_each
    }//end if
    
    return;
}

static void threadpool_free(threadpool_t *pool)
{
    int i = 0;
    if (pool->join_list) {
         if ( !list_empty(pool->join_list)) {
            recover_clean_queue(pool);
         }

        free(pool->join_list);
    }

    if (pool->task_queue) {
        free(pool->task_queue);
    }//end if

    if (pool->threads) {
        free(pool->threads);
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));

        pthread_mutex_lock(&(pool->thread_counter));
        pthread_mutex_destroy(&(pool->thread_counter));

        pthread_cond_destroy(&(pool->queue_not_empty));
        pthread_cond_destroy(&(pool->queue_not_full));

    }//end if
    //free(pool);

    //pool = NULL;// 无效做法.
}

/* 寻找空闲位置 */
static int find_free_task(threadpool_t *pool)  
{
    int i;
    int res = pool->queue_free ;
    for (i = 0; i < pool->queue_max_size 
        && pool->task_queue[(res + i)%pool->queue_max_size].status == ON; i++)
        ;
    if (i != pool->queue_max_size) {
        if (pool->task_queue[(res + i)%pool->queue_max_size].status == OFF)
        res =(res + i)%pool->queue_max_size;

        return res;
    }
}

/* 寻找有任务的位置 */
static int  find_work_task(threadpool_t *pool)
{
    int i;
    int res = pool->queue_work;
    for (i = 0; i < pool->queue_max_size
        && pool->task_queue[(res + i)%pool->queue_max_size].status == OFF; i++)
        ;
    if (i != pool->queue_max_size) {
        if(pool->task_queue[(res + i)%pool->queue_max_size].status == ON)
        res =(res + i)%pool->queue_max_size;

        return res;
    }
}


//把任务添加到队列中
int threadpool_add(threadpool_t *pool, void*(*function)(void *arg), void *arg)
{
    if (pool == NULL || function == NULL || arg == NULL)
        err_quit("value fail.");
    _Argument *args = (_Argument *)arg;

    pthread_mutex_lock(&(pool->lock));
    while ((pool->queue_size == pool->queue_max_size) && (!pool->shutdown))    {
        //对列满，等待...
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
    }

    if (pool->shutdown) {
        pthread_mutex_unlock(&(pool->lock));
    }

    pool->queue_free = find_free_task(pool);//寻找空闲位置

    pool->task_queue[pool->queue_free].function = function;
    pool->task_queue[pool->queue_free].arg = *((_Argument *)arg);
    pool->task_queue[pool->queue_free].status = ON; //激活任务
    pool->queue_size++;

    //每次加完任务，发个信号给线程,若没有线程处于等待状态，则忽略
    pthread_cond_signal(&(pool->queue_not_empty));

    pthread_mutex_unlock(&(pool->lock));

    return 0;
}

//执行线程任务
static void *threadpool_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;
    int index;

    //task.arg = (_Argument *)malloc(sizeof(_Argument));

    while (TRUE) {
        bzero(&task.arg, sizeof(_Argument));
        pthread_mutex_lock(&pool->lock);
        //任务队列为空的时候，等待....
        while ((pool->queue_size == 0) && (!pool->shutdown)) {
            printf("thread %d is waitting...\n", pthread_self());
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));
            //被唤醒后，判断是否是要退出的线程
            if (pool->wait_exit_thr_num > 0) {
                pool->wait_exit_thr_num--;
                if (pool->live_thr_num > pool->min_thr_num) {
                    bzero(&task.arg, sizeof(_Argument));
                    
                    printf("减少线程: %d\n", pthread_self());

                    index = find_thread_index(pool, pthread_self());
                    pool->threads[index] = 0;//置回0
                    pool->live_thr_num--;
                    put_clean_queue(pool, pthread_self());
                    
                    pthread_mutex_unlock(&(pool->lock));
                    pthread_exit(NULL);
                }
             }
        }
        if (pool->shutdown) {
           pthread_mutex_unlock(&(pool->lock));
           printf("thread %d is exiting\n", pthread_self());
           put_clean_queue(pool, pthread_self());
           pthread_exit(NULL);
        }
        /* get a task from queue  */
        pool->queue_work = find_work_task(pool);
        task =  pool->task_queue[pool->queue_work];
        bzero(&(pool->task_queue[pool->queue_work].arg),sizeof(_Argument));
        pool->task_queue[pool->queue_work].status = OFF; //置为空闲
        pool->queue_size--;

        /* now queue must be not full. */
        pthread_cond_broadcast(&(pool->queue_not_full));
        pthread_mutex_unlock(&(pool->lock));

        //printf("thread %d is working...\n", pthread_self());
        pthread_mutex_lock(&(pool->thread_counter));
        pool->busy_thr_num++;
        pthread_mutex_unlock(&(pool->thread_counter));

        /* task run over */
        (*(task.function))(&task.arg);

        //printf("thread %d end working.\n", pthread_self());
        pthread_mutex_lock(&(pool->thread_counter));
        pool->busy_thr_num--;
        pthread_mutex_unlock(&(pool->thread_counter));
    }
    pthread_exit(NULL);

    return (NULL);
}

//管理线程
static void *adjust_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    while (!pool->shutdown) {
        sleep(DEFAULT_TIME);

        pthread_mutex_lock(&(pool->lock));
        int queue_size = pool->queue_size;
        int live_thr_num = pool->live_thr_num;
        pthread_mutex_unlock(&(pool->lock));

        pthread_mutex_lock(&(pool->thread_counter));
        int busy_thr_num = pool->busy_thr_num;
        pthread_mutex_unlock(&(pool->thread_counter));

        //任务多线程少，增加线程
        if (queue_size >= MIN_WAIT_TASK_NUM
        &&live_thr_num < pool->max_thr_num) {
            /* need add thread. */
            pthread_mutex_lock(&(pool->lock));
            int i, add = 0;
            for (i = 0; i < pool->max_thr_num && add < DEFAULT_THREAD_VARY
            && pool->live_thr_num < pool->max_thr_num; i++) {
                if (pool->threads[i] == 0 
                || !is_thread_alive(pool->threads[i])) {
                    pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
                    add++;
                    pool->live_thr_num++;
                }
            }
            pthread_mutex_unlock(&(pool->lock));
        }

        //任务少，线程多,减少线程.
        if ((busy_thr_num * 2) < live_thr_num 
        && live_thr_num > pool->min_thr_num) {
            /* need del thread */
            pthread_mutex_lock(&(pool->lock));
            pool->wait_exit_thr_num = DEFAULT_THREAD_VARY;
            pthread_mutex_unlock(&(pool->lock));
            /* wake up thread to exit */
            int j;
            for (j = 0; j < DEFAULT_THREAD_VARY; j++) {
                pthread_cond_signal(&(pool->queue_not_empty));
            }//end for

            sleep(3);
            pthread_mutex_lock(&(pool->lock));
            recover_clean_queue(pool);             /* 回收线程 */
            pthread_mutex_unlock(&(pool->lock));
        }//end if
    }//end while
}

int threadpool_destroy(threadpool_t *pool)
{
    int i;
    if(pool == NULL)
        return NG;
    pool->shutdown = TRUE;

    /* adjust_tid exit first */
    pthread_join(pool->adjust_tid, NULL);

    /* wake up the waiting thread */
    pthread_cond_broadcast(&(pool->queue_not_empty));
    sleep(3);

    pthread_mutex_lock(&(pool->lock));
    recover_clean_queue(pool);
    pthread_mutex_unlock(&(pool->lock));
    
    threadpool_free(pool);

    return OK;
}

static int threadpool_all_threadnum(threadpool_t *pool)
{
    int all_threadnum = -1;
    pthread_mutex_lock(&(pool->lock));
    all_threadnum = pool->live_thr_num;
    pthread_mutex_unlock(&(pool->lock));

    return all_threadnum;
}

static int threadpool_busy_threadnum(threadpool_t *pool)
{
    int busy_thr_num = -1;
    pthread_mutex_lock(&(pool->thread_counter));
    busy_thr_num = pool->busy_thr_num;
    pthread_mutex_unlock(&(pool->thread_counter));

    return (busy_thr_num);
}

static bool is_thread_alive(pthread_t tid)
{
    int kill_rc = pthread_kill(tid, 0);

    if (kill_rc == ESRCH) {
        return FALSE;
    }

    return TRUE;
}
