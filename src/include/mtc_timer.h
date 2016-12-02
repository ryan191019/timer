
/*
 * @file mtc_timer.h
 *
 * @author  Frank Wang
 * @date    2015-10-14
 */

#ifndef __MTC_TIMER_H__
#define __MTC_TIMER_H__

#include <pthread.h>

#include "mtc_linklist.h"
#include "mtc_ipc.h"

#define PTHREAD_SET_UNATTACH_ATTR(attr) \
    pthread_attr_init(&attr); \
    pthread_attr_setdetachstate(&attr,PTHREAD_CREATE_DETACHED)

/**
 * @define typecheck
 * @brief check type
 */
#define typecheck(type,x) \
({  type __dummy; \
    typeof(x) __dummy2; \
    (void)(&__dummy == &__dummy2); \
    1; \
})

/**
 * @define time_after(a, b)
 * @brief is b bigger than a
 */
#define time_after(a,b) \
    (typecheck(unsigned long, a) && \
     typecheck(unsigned long, b) && \
     ((long)(b) - (long)(a) < 0))
/**
 * @define time_before(a, b)
 * @brief is b smller than a
 */
#define time_before(a,b)    time_after(b,a)

/**
 * @define time_after_eq(a, b)
 * @brief is b bigger or equal than a
 */
#define time_after_eq(a,b) \
    (typecheck(unsigned long, a) && \
     typecheck(unsigned long, b) && \
     ((long)(a) - (long)(b) >= 0))
/**
 * @define time_before_eq(a, b)
 * @brief is b smller or equal than a
 */
#define time_before_eq(a,b) time_after_eq(b,a)

/**This function be called when a timer expire*/
typedef void (*timer_func)(int sData, void *data);

/**
 * @struct mytimer_t
 * @brief timer struct
 */
typedef struct
{
    struct list_head    list;
    timer_func          func;
    int                 sData;
    void                *data;  /*!< must can be free()!!!*/
    unsigned long       expire; /*!< jiffies */
}mytimer_t;

/**
 * @struct tThreadCb
 * @brief thread struct
 */
typedef struct 
{
    int semid;
    pthread_t id;
}tThreadCb;

/**
 *  @brief Get current time. Must support jiffer export in /proc file system!!
 *  @param *currentTime when get time successed, you will get current time in this field.
 *
 *  @return non-zero for success and 0 for failed.
 */
unsigned int getCurrentTime(unsigned int *currentTime);

/**
 *  @brief Create timer thread. Only one timer thread can be create in the one process.
 *  @param *cb Must give a valid semaphore id. 
 *
 *  @return TRUE for success and FALSE for failed.
 */
unsigned char createTimerThread(tThreadCb *cb); //must give a valid semaphore id

/**
 *  @brief Destroy timer thread
 *
 *  @return None.
 */
void destoryTimerThread();

/* Timer Operator */

/**
 *  @brief Add timer
 *  @param *timer Timer user defined.
 *  @return None.
 */
void addTimer(mytimer_t *timer);

void __delTimer(mytimer_t *timer);

/**
 *  @brief Delete timer
 *  @param *timer Timer you want to delete. 
 *  @return None.
 */
void delTimer(mytimer_t *timer);

void __modTimer(mytimer_t *timer, unsigned int expire); //w/o lock version

/**
 *  @brief Modify timer
 *  @param *timer Timer you want to delete. 
 *  @param expire The new expire you want add to timer.
 *  @return None.
 */
void modTimer(mytimer_t *timer, unsigned int expire);

#endif

