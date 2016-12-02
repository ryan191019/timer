#ifndef _MTC_THREADPOOL_H_
#define _MTC_THREADPOOL_H_

/**
* @file threadpool.h
* @brief Threadpool Header File
*/

typedef struct threadpool_t threadpool_t;

typedef enum 
{
    threadpool_success = 0,
    threadpool_invalid = -1,
    threadpool_lock_failure = -2,
    threadpool_queue_full = -3,
    threadpool_shutdown = -4,
    threadpool_thread_failure = -5
} threadpool_error_t;

typedef enum 
{
    threadpool_graceful = 1
} threadpool_destroy_flags_t;

/**
* @function threadpool_init
* @brief Creates a threadpool_t object.
* @param thread_count Number of worker threads.
* @param queue_size Size of the queue.
* @param flags Unused parameter.
* @return a newly created thread pool or NULL
*/
threadpool_t *threadpool_init(int thread_count, int queue_size, int flags);

/**
* @function task_enqueue
* @brief add a new task in the queue of a thread pool
* @param pool Thread pool to which add the task.
* @param function Pointer to the function that will perform the task.
* @param argument Argument to be passed to the function.
* @param flags Unused parameter.
* @return 0 if all goes well, negative values in case of error (@see
* threadpool_error_t for codes).
*/
int task_enqueue(threadpool_t *pool, void (*routine)(void *),
                   void *arg, int flags);

/**
* @function threadpool_destroy
* @brief Stops and destroys a thread pool.
* @param pool Thread pool to destroy.
* @param flags Flags for shutdown
*
* Known values for flags are 0 (default) and threadpool_graceful in
* which case the thread pool doesn't accept any new tasks but
* processes all pending tasks before shutdown.
*/
int threadpool_destroy(threadpool_t *pool, int flags);


//========================================================================
typedef enum 
{
    threadlimit_success = 0,
    threadlimit_lock_failure = -1,
    threadlimit_failure = -2,
} threadlimit_error_t;

typedef struct threadlimit_t threadlimit_t;

threadlimit_t *threadlimit_init(int thread_max, int flags);
int threadlimit_free(threadlimit_t *limit);
int threadlimit_inc(threadlimit_t *limit);
int threadlimit_dec(threadlimit_t *limit);


#endif /* _THREADPOOL_H_ */
