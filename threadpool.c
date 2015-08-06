#include "threadpool.h"

#if 1
void *threadpool_thread(void *threadpool);
void *adjust_thread(void *threadpool);
bool is_thread_alive(pthread_t tid);
void threadpool_free(threadpool_t *pool);
int  find_free_task(threadpool_t *pool);
int  find_work_task(threadpool_t *pool);
void put_claen_queue(threadpool_t *pool, pthread_t id);
void recover_clean_queue(threadpool_t *pool);
int  find_thread_index(threadpool_t *pool, pthread_t id);
#endif

//创建线程池
threadpool_t *threadpool_create(int min_thr_num, int max_thr_num, int queue_max_size)
{
    threadpool_t *pool = NULL;

    do {
        if((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) {
            printf("malloc threadpool fail.");
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

        if(pool->threads == NULL) {
            printf("malloc threads fail.\n");
            break;
        }
        memset(pool->threads, 0, sizeof(pool->threads));

        pool->queue_join = (thread_join *)malloc(sizeof(thread_join));
        if(pool->queue_join == NULL) {
            printf("malloc queue_join fail.\n");
            break;
        }
        pool->queue_join->cnt = 0;
        pool->queue_join->head = pool->queue_join->tail = NULL;

        //创建任务队列
        pool->task_queue = (threadpool_task_t *)malloc(sizeof(threadpool_task_t)*queue_max_size);
        if(pool->task_queue == NULL) {
            printf("malloc task_queue fail.\n");
            break;
        }
        //初始化任务队列...
        int i;
        for(i = 0; i < queue_max_size; i++) {
             pool->task_queue[i].arg = (_Argument *)malloc(sizeof(_Argument));
             pool->task_queue[i].status = OFF; //no use
        }
        //初始化锁与条件变量
        if(pthread_mutex_init(&(pool->lock), NULL) != 0
        || pthread_mutex_init(&(pool->thread_counter), NULL) != 0
        || pthread_cond_init(&(pool->queue_not_empty), NULL) != 0
        || pthread_cond_init(&(pool->queue_not_full), NULL ) != 0)
        {
            printf("init the lock or cond fail.");
            break;
        }
        //启动任务线程
        for(i = 0; i < min_thr_num; i++)
        {
            pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
            printf("start thread %d...", pool->threads[i]);
        }
        //启动管理线程
        pthread_create(&(pool->adjust_tid), NULL, adjust_thread, (void *)pool);

        return pool;
    }while(0);

    threadpool_free(pool);
    return NULL;
}

int find_thread_index(threadpool_t *pool, pthread_t id)
{
    int i;
    for(i = 0; i < pool->max_thr_num && id != pool->threads[i]; i++)
        ;
    return i;
}

//放入回收链表
void put_claen_queue(threadpool_t *pool, pthread_t id)
{
    exit_thr_node *node = (exit_thr_node *)malloc(sizeof(exit_thr_node));
    node->next = NULL;
    node->id   = id;

    if(pool->queue_join->cnt == 0)
        pool->queue_join->head = node;
    else 
        pool->queue_join->tail->next = node;
    pool->queue_join->tail = node;
    pool->queue_join->cnt++;
}

void recover_clean_queue(threadpool_t *pool)
{
    int i;
    exit_thr_node *temp;
    
    if(pool->queue_join->cnt) {
        while(pool->queue_join->cnt && pool->queue_join->head) {
            temp = pool->queue_join->head->next;
            printf("线程被回收 %d\n", pool->queue_join->head->id);
            if(pthread_join(pool->queue_join->head->id, NULL))
                printf("线程回收失败, %d\n", pool->queue_join->head->id);
            free(pool->queue_join->head);
            pool->queue_join->cnt--;
            pool->queue_join->head = temp;
        }
        pool->queue_join->tail = NULL;
    }
}

void threadpool_free(threadpool_t *pool)
{
    int i = 0;
    if(pool->queue_join) {
        if(pool->queue_join->cnt) {
            recover_clean_queue(pool);       
        }
        free(pool->queue_join);
    }

    if(pool->task_queue) {
        if(pool->task_queue[0].arg != NULL) {
            for(i = 0; i < pool->queue_max_size && pool->task_queue[i].arg != NULL; i++) {
                 free(pool->task_queue[i].arg);
            }//end for
        }//end if
        free(pool->task_queue);
    }//end if

    if(pool->threads) {
        free(pool->threads);
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));

        pthread_mutex_lock(&(pool->thread_counter));
        pthread_mutex_destroy(&(pool->thread_counter));

        pthread_cond_destroy(&(pool->queue_not_empty));
        pthread_cond_destroy(&(pool->queue_not_full));

    }//end if
    free(pool);

    pool = NULL;// 无效做法.
}

/* 寻找空闲位置 */
int find_free_task(threadpool_t *pool)  
{
    int i;
    int res = pool->queue_free ;
    for(i = 0; i < pool->queue_max_size 
        && pool->task_queue[(res + i)%pool->queue_max_size].status == ON; i++)
        ;
    if(i != pool->queue_max_size) {
        if(pool->task_queue[(res + i)%pool->queue_max_size].status == OFF)
        res =(res + i)%pool->queue_max_size;

        return res;
    }
}

/* 寻找有任务的位置 */
int  find_work_task(threadpool_t *pool)
{
    int i;
    int res = pool->queue_work;
    for(i = 0; i < pool->queue_max_size
        && pool->task_queue[(res + i)%pool->queue_max_size].status == OFF; i++)
        ;
    if(i != pool->queue_max_size) {
        if(pool->task_queue[(res + i)%pool->queue_max_size].status == ON)
        res =(res + i)%pool->queue_max_size;

        return res;
    }
}


//把任务添加到队列中
int threadpool_add(threadpool_t *pool, void*(*function)(void *arg), void *arg)
{
    if(pool == NULL || function == NULL || arg == NULL)
        err_quit("value fail.");
    _Argument *args = (_Argument *)arg;

    pthread_mutex_lock(&(pool->lock));
    while((pool->queue_size == pool->queue_max_size) && (!pool->shutdown)) {
        //对列满，等待...
        pthread_cond_wait(&(pool->queue_not_full), &(pool->lock));
    }

    if(pool->shutdown) {
        pthread_mutex_unlock(&(pool->lock));
    }

    pool->queue_free = find_free_task(pool);//寻找空闲位置

    pool->task_queue[pool->queue_free].function = function;
    ((_Argument *)(pool->task_queue[pool->queue_free].arg))->pid = args->pid;
    ((_Argument *)(pool->task_queue[pool->queue_free].arg))->port = args->port;
    if(args->ip != NULL)
        memcpy(((_Argument *)(pool->task_queue[pool->queue_free].arg))->ip,args->ip,IP_LENGTH);
    else
        ;//让他空着
    pool->task_queue[pool->queue_free].status = ON; //激活任务
    pool->queue_size++;

    //每次加完任务，发个信号给线程,若没有线程处于等待状态，则忽略
    pthread_cond_signal(&(pool->queue_not_empty));

    pthread_mutex_unlock(&(pool->lock));

    return 0;
}

//执行线程任务
void *threadpool_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;
    int index;

    task.arg = (_Argument *)malloc(sizeof(_Argument));

    while(TRUE) {
        bzero(task.arg, sizeof(_Argument));
        pthread_mutex_lock(&pool->lock);
        //任务队列为空的时候，等待....
        while ((pool->queue_size == 0) && (!pool->shutdown)) {
            printf("thread %d is waitting...\n", pthread_self());
            pthread_cond_wait(&(pool->queue_not_empty), &(pool->lock));
            //被唤醒后，判断是否是要退出的线程
            if(pool->wait_exit_thr_num > 0) {
                pool->wait_exit_thr_num--;
                if(pool->live_thr_num > pool->min_thr_num) {
                    free(task.arg);
                    
                    printf("减少线程: %d\n", pthread_self());

                    index = find_thread_index(pool, pthread_self());
                    pool->threads[index] = 0;//置回0
                    pool->live_thr_num--;
                    put_claen_queue(pool, pthread_self());
                    
                    pthread_mutex_unlock(&(pool->lock));
                    pthread_exit(NULL);
                }
             }
        }
        if(pool->shutdown) {
           pthread_mutex_unlock(&(pool->lock));
           printf("thread %d is exiting\n", pthread_self());
           free(task.arg);
           put_claen_queue(pool, pthread_self());
           pthread_exit(NULL);
        }
        //get a task from queue
        pool->queue_work = find_work_task(pool);

        task.function = pool->task_queue[pool->queue_work].function;
        ((_Argument *)(task.arg))->port = ((_Argument *)(pool->task_queue[pool->queue_work].arg))->port;
        memcpy(((_Argument *)(task.arg))->ip,((_Argument *)(pool->task_queue[pool->queue_work].arg))->ip,IP_LENGTH);
        
        bzero(pool->task_queue[pool->queue_work].arg, sizeof(_Argument));
        pool->task_queue[pool->queue_work].status = OFF; //置为空闲
        pool->queue_size--;

        //now queue must be not full.
        pthread_cond_broadcast(&(pool->queue_not_full));
        pthread_mutex_unlock(&(pool->lock));

        printf("thread %d is working...\n", pthread_self());
        pthread_mutex_lock(&(pool->thread_counter));
        pool->busy_thr_num++;
        pthread_mutex_unlock(&(pool->thread_counter));

        //task run over
        (*(task.function))(task.arg);

        printf("thread %d end working.\n", pthread_self());
        pthread_mutex_lock(&(pool->thread_counter));
        pool->busy_thr_num--;
        pthread_mutex_unlock(&(pool->thread_counter));
    }
    pthread_exit(NULL);

    return (NULL);
}

//管理线程
void *adjust_thread(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    while(!pool->shutdown) {
        sleep(DEFAULT_TIME);

        pthread_mutex_lock(&(pool->lock));
        int queue_size = pool->queue_size;
        int live_thr_num = pool->live_thr_num;
        pthread_mutex_unlock(&(pool->lock));

        pthread_mutex_lock(&(pool->thread_counter));
        int busy_thr_num = pool->busy_thr_num;
        pthread_mutex_unlock(&(pool->thread_counter));

        //任务多线程少，增加线程
        if(queue_size >= MIN_WAIT_TASK_NUM
        &&live_thr_num < pool->max_thr_num) {
            //need add thread.
            pthread_mutex_lock(&(pool->lock));
            int i, add = 0;
            for(i = 0; i < pool->max_thr_num && add < DEFAULT_THREAD_VARY
            && pool->live_thr_num < pool->max_thr_num; i++) {
                if(pool->threads[i] == 0 
                || !is_thread_alive(pool->threads[i])) {
                    pthread_create(&(pool->threads[i]), NULL, threadpool_thread, (void *)pool);
                    add++;
                    pool->live_thr_num++;
                }
            }
            pthread_mutex_unlock(&(pool->lock));
        }

        //任务少，线程多,减少线程.
        if((busy_thr_num * 2) < live_thr_num 
        && live_thr_num > pool->min_thr_num) {
            //need del thread
            pthread_mutex_lock(&(pool->lock));
            pool->wait_exit_thr_num = DEFAULT_THREAD_VARY;
            pthread_mutex_unlock(&(pool->lock));
            //wake up thread to exit
            int j;
            for(j = 0; j < DEFAULT_THREAD_VARY; j++) {
                pthread_cond_signal(&(pool->queue_not_empty));
            }//end for

            sleep(3);
            pthread_mutex_lock(&(pool->lock));
            recover_clean_queue(pool);//回收线程
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

    //adjust_tid exit first
    pthread_join(pool->adjust_tid, NULL);

    //wake up the waiting thread
    pthread_cond_broadcast(&(pool->queue_not_empty));
    sleep(3);

    pthread_mutex_lock(&(pool->lock));
    recover_clean_queue(pool);
    pthread_mutex_unlock(&(pool->lock));
    
    threadpool_free(pool);

    return OK;
}

int threadpool_all_threadnum(threadpool_t *pool)
{
    int all_threadnum = -1;
    pthread_mutex_lock(&(pool->lock));
    all_threadnum = pool->live_thr_num;
    pthread_mutex_unlock(&(pool->lock));

    return all_threadnum;
}

int threadpool_busy_threadnum(threadpool_t *pool)
{
    int busy_thr_num = -1;
    pthread_mutex_lock(&(pool->thread_counter));
    busy_thr_num = pool->busy_thr_num;
    pthread_mutex_unlock(&(pool->thread_counter));

    return busy_thr_num;
}

bool is_thread_alive(pthread_t tid)
{
    int kill_rc = pthread_kill(tid, 0);

    if(kill_rc == ESRCH) {
        return FALSE;
    }

    return TRUE;
}
