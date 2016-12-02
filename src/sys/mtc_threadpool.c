/**
* @file threadpool.c
* @brief Threadpool implementation file
*/

#include <stdlib.h>
#include <pthread.h>
#include <unistd.h>
#include <stdio.h>

#include "include/mtc.h"
#include "include/mtc_misc.h"
#include "include/mtc_threadpool.h"

typedef enum 
{
    immediate_shutdown = 1,
    graceful_shutdown = 2
} threadpool_shutdown_t;

/**
* @struct threadpool_task
* @brief the work struct
*
* @var function Pointer to the function that will perform the task.
* @var argument Argument to be passed to the function.
*/

typedef struct 
{
    void (*function)(void *);
    void *argument;
} threadpool_task_t;

/**
* @struct threadpool
* @brief The threadpool struct
*
* @var notify Condition variable to notify worker threads.
* @var threads Array containing worker threads ID.
* @var thread_count Number of threads
* @var queue Array containing the task queue.
* @var queue_size Size of the task queue.
* @var head Index of the first element.
* @var tail Index of the next element.
* @var count Number of pending tasks
* @var shutdown Flag indicating if the pool is shutting down
* @var started Number of started threads
*/
struct threadpool_t 
{
    pthread_mutex_t lock;
    pthread_cond_t notify;
    pthread_t *threads;
    threadpool_task_t *queue;
    int thread_count;
    int queue_size;
    int head;
    int tail;
    int count;
    int shutdown;
    int started;
};

/**
* @function void *threadpool_threadwork(void *threadpool)
* @brief the worker thread
* @param threadpool the pool which own the thread
*/
static void *threadpool_threadwork(void *threadpool);

int threadpool_free(threadpool_t *pool);

threadpool_t *threadpool_init(int thread_count, int queue_size, int flags)
{
    threadpool_t *pool;
    int i;

    /* TODO: Check for negative or otherwise very big input parameters */

    if((pool = (threadpool_t *)malloc(sizeof(threadpool_t))) == NULL) {
        goto err;
    }

    /* Initialize */
    pool->thread_count = 0;
    pool->queue_size = queue_size;
    pool->head = pool->tail = pool->count = 0;
    pool->shutdown = pool->started = 0;

    /* Allocate thread and task queue */
    pool->threads = (pthread_t *)malloc(sizeof(pthread_t) * thread_count);
    pool->queue = (threadpool_task_t *)malloc
        (sizeof(threadpool_task_t) * queue_size);

    /* Initialize mutex and conditional variable first */
    if((pthread_mutex_init(&(pool->lock), NULL) != 0) ||
       (pthread_cond_init(&(pool->notify), NULL) != 0) ||
       (pool->threads == NULL) ||
       (pool->queue == NULL)) {
        goto err;
    }

    /* Start worker threads */
    for(i = 0; i < thread_count; i++) {
        if (flags != 0) {
            pthread_attr_t attr;	
            pthread_attr_init(&attr);
            pthread_attr_setstacksize(&attr, flags * 1024);

            if(pthread_create(&(pool->threads[i]), &attr, threadpool_threadwork, (void*)pool) != 0) {
                threadpool_destroy(pool, 0);
                return NULL;
            }
        }
        else {
            if(pthread_create(&(pool->threads[i]), NULL, threadpool_threadwork, (void*)pool) != 0) {
                threadpool_destroy(pool, 0);
                return NULL;
            }

        }


        pool->thread_count++;
        pool->started++;

        //stdprintf("pool->thread_count = %d\n",pool->thread_count);
    }

    //stdprintf("pool@%p\n",pool);

    return pool;

 err:
    if(pool) {
        threadpool_free(pool);
    }
    return NULL;
}

int task_enqueue(threadpool_t *pool, void (*function)(void *),
                   void *argument, int flags)
{
    int ret = threadpool_success;
    int next;

    if(pool == NULL || function == NULL) {
        return threadpool_invalid;
    }

    if(pthread_mutex_lock(&(pool->lock)) != 0) {
        return threadpool_lock_failure;
    }

    next = pool->tail + 1;
    next = (next == pool->queue_size) ? 0 : next;

    do {
        /* Are we full ? */
        if(pool->count == pool->queue_size) {
            ret = threadpool_queue_full;
            printf("queue full!!!!\n");
            break;
        }

        /* Are we shutting down ? */
        if(pool->shutdown) {
            ret = threadpool_shutdown;
            printf("shutting down!!!!\n");
            break;
        }

        /* Add task to queue */
        pool->queue[pool->tail].function = function;
        pool->queue[pool->tail].argument = argument;
        pool->tail = next;
        pool->count++;

        /* pthread_cond_broadcast */
        if(pthread_cond_signal(&(pool->notify)) != 0) {
            ret = threadpool_lock_failure;
            break;
        }
    } while(0);

    if(pthread_mutex_unlock(&pool->lock) != 0) {
        ret = threadpool_lock_failure;
    }

    return ret;
}

int threadpool_destroy(threadpool_t *pool, int flags)
{
    int i, ret = threadpool_success;

    if(pool == NULL) {
        return threadpool_invalid;
    }

    if(pthread_mutex_lock(&(pool->lock)) != 0) {
        return threadpool_lock_failure;
    }

    do {
        /* Already shutting down */
        if(pool->shutdown) {
            ret = threadpool_shutdown;
            break;
        }

        pool->shutdown = (flags & threadpool_graceful) ?
            graceful_shutdown : immediate_shutdown;

        /* Wake up all worker threads */
        if((pthread_cond_broadcast(&(pool->notify)) != 0) ||
           (pthread_mutex_unlock(&(pool->lock)) != 0)) {
            ret = threadpool_lock_failure;
            break;
        }

        /* Join all worker thread */
        for(i = 0; i < pool->thread_count; i++) {
            if(pthread_join(pool->threads[i], NULL) != 0) {
                ret = threadpool_thread_failure;
            }
        }
    } while(0);

    /* Only if everything went well do we deallocate the pool */
    if(!ret) {
        threadpool_free(pool);
    }
    return ret;
}

int threadpool_free(threadpool_t *pool)
{
    if(pool == NULL || pool->started > 0) {
        return -1;
    }

    /* Did we manage to allocate ? */
    if(pool->threads) {
        free(pool->threads);
        free(pool->queue);
 
        /* Because we allocate pool->threads after initializing the
           mutex and condition variable, we're sure they're
           initialized. Let's lock the mutex just in case. */
        pthread_mutex_lock(&(pool->lock));
        pthread_mutex_destroy(&(pool->lock));
        pthread_cond_destroy(&(pool->notify));
    }
    free(pool);
    return 0;
}


static void *threadpool_threadwork(void *threadpool)
{
    threadpool_t *pool = (threadpool_t *)threadpool;
    threadpool_task_t task;

    for(;;) {
        /* Lock must be taken to wait on conditional variable */
        pthread_mutex_lock(&(pool->lock));

        /* Wait on condition variable, check for spurious wakeups.
           When returning from pthread_cond_wait(), we own the lock. */
        while((pool->count == 0) && (!pool->shutdown)) {
            pthread_cond_wait(&(pool->notify), &(pool->lock));
        }

        if((pool->shutdown == immediate_shutdown) ||
           ((pool->shutdown == graceful_shutdown) &&
            (pool->count == 0))) {
            break;
        }

        /* Grab our task */
        task.function = pool->queue[pool->head].function;
        task.argument = pool->queue[pool->head].argument;
        pool->head++;
        pool->head = (pool->head == pool->queue_size) ? 0 : pool->head;
        pool->count--;

        /* Unlock */
        pthread_mutex_unlock(&(pool->lock));

        /* Get to work */
        (*(task.function))(task.argument);
    }

    pool->started--;

    pthread_mutex_unlock(&(pool->lock));
    pthread_exit(NULL);
    return(NULL);
}

struct threadlimit_t 
{
    pthread_mutex_t lock;
    int thread_count;
    int thread_max;
};

#define THREAD_LIMIT_DEBUG  0

threadlimit_t *threadlimit_init(int thread_max, int flags)
{
    threadlimit_t *limit;

    if((limit = (threadlimit_t *)malloc(sizeof(threadlimit_t))) == NULL) {
        goto err;
    }

    /* Initialize */
    limit->thread_count = 0;
    limit->thread_max = thread_max;

    if(pthread_mutex_init(&(limit->lock), NULL) != 0) {
        goto err;
    }

    //stdprintf("limit@%p\n",limit);

    return limit;

 err:
    if(limit) {
        threadlimit_free(limit);
    }
    return NULL;
}

int threadlimit_free(threadlimit_t *limit)
{
    if(limit == NULL) {
        return threadlimit_failure;
    }
 
    pthread_mutex_lock(&(limit->lock));
    pthread_mutex_destroy(&(limit->lock));

    free(limit);
    return 0;
}

int threadlimit_inc(threadlimit_t *limit)
{
    int ret = threadlimit_success;

    if(limit == NULL) {
        return threadlimit_failure;
    }

    if(pthread_mutex_lock(&(limit->lock)) != 0) {
        return threadlimit_failure;
    }

    if (limit->thread_count < limit->thread_max) {
        limit->thread_count+=1;
#if THREAD_LIMIT_DEBUG
        stdprintf("current thread_count = %d\n",limit->thread_count);
#endif
    }
    else {
        ret = threadlimit_failure;
    }

    if(pthread_mutex_unlock(&limit->lock) != 0) {
        ret = threadpool_lock_failure;
    }

    return ret;
}

int threadlimit_dec(threadlimit_t *limit)
{
    int ret = threadlimit_success;

    if(limit == NULL) {
        return threadlimit_failure;
    }

    if(pthread_mutex_lock(&(limit->lock)) != 0) {
        return threadlimit_failure;
    }

    if (limit->thread_count > 0) {
        limit->thread_count-=1;
#if THREAD_LIMIT_DEBUG
        stdprintf("current thread_count = %d\n",limit->thread_count);
#endif
    }
    else {
        ret = threadlimit_failure;
    }

    if(pthread_mutex_unlock(&limit->lock) != 0) {
        ret = threadpool_lock_failure;
    }

    return ret;
}



