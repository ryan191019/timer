
/**
 * @file mtc_timer.c   Timer Relation Function
 *
 * @author  Frank Wang
 * @date    2015-10-14
 */
#include <string.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include "include/mtc.h"
#include "include/mtc_timer.h"
#include "include/mtc_ipc.h"

/**
 * @def TIMERDEBUG
 * @brief The config of debug. default as closed.
 */
#define TIMERDEBUG      0

#if TIMERDEBUG
#define timer_lock(semid)   down_debug(semid)
#define timer_unlock(semid) up_debug(semid)
#else
#define timer_lock(semid)   down(semid)
#define timer_unlock(semid) up(semid)
#endif

static unsigned char timerThreadAlreadyLaunch = FALSE;
static int socket_pair[2];
static LIST_HEAD(timerListRunning);
static LIST_HEAD(timerListExpired);
static LIST_HEAD(timerListExec);
static tThreadCb    tCb;

/**
 *  @brief Get absolute time in second
 *  @param *currentTime when get time successed, you will get current time in this field.
 *
 *  @return non-zero for success and 0 for failed.
 */
unsigned int getCurrentTime(unsigned int *currentTime)
{
    struct timespec ts;	
    clock_gettime(CLOCK_MONOTONIC, &ts);

    if (currentTime)
        *currentTime = ts.tv_sec;

    return ts.tv_sec;
}

/**
 *  @enum operatorName
 *  @brief Operator code
 */
typedef enum
{
    timer_addTimer = 0, /**< Add timer */
    timer_delTimer,     /**< Delete timer */
    timer_modTimer,     /**< Modify timer */
    timer_reSchedule,   /**< Reschedule */
}operatorName;

static void sendNotifyToTimerThread(operatorName caller)
{
    int sig = caller;
reSend:
    if (send(socket_pair[1], &sig, sizeof(sig), MSG_DONTWAIT) < 0) {
        if ( errno == EINTR || errno == EAGAIN ) {
            goto reSend;    /* try again */
        }
        stdprintf("[%d]Could not send signal: %d\n", caller, sig);
    }
#if TIMERDEBUG
    stdprintf("[%d]Send signal: %d\n", caller, sig);
#endif
}

#define TIMERFUNCDEBUG  0

static void *processTimerListThread(void *arg)
{
    struct list_head *pos, *n;
    mytimer_t *timer;

#if TIMERDEBUG || TIMERFUNCDEBUG
    stdprintf("Thread Create[processTimerListThread]\n");
#endif

    for(;;) {
        timer_lock(tCb.semid);
        if (!list_empty(&timerListExec)) {
            list_for_each_safe(pos,n,&timerListExec) {
                list_move(pos, &timerListExpired);
                break;
            }
        }
        else {
            timer_unlock(tCb.semid);
            goto exit;
        }
        timer_unlock(tCb.semid);

        timer = (mytimer_t *)list_entry(pos, mytimer_t, list);
#if TIMERFUNCDEBUG
        stdprintf("Start run %p\n",timer->func);
#endif
        timer->func(timer->sData, timer->data);
#if TIMERFUNCDEBUG
        stdprintf("End   run %p\n",timer->func);
#endif

    }

exit:
    pthread_exit(0);
}

//call sem down before this func calling
static void checkTimerList(unsigned long now)
{
    struct list_head *pos, *n;

    if (list_empty(&timerListRunning))
        return;

    list_for_each_safe(pos,n,&timerListRunning) {
        mytimer_t *timer = (mytimer_t *)list_entry(pos, mytimer_t, list);

        if (time_after_eq(now,timer->expire)) {
            list_move(pos, &timerListExec);
        }
    }

    return;
}

//call sem down before this func calling
static unsigned int checkNearestTimeout(unsigned int now)
{
    if (list_empty(&timerListRunning)) {
        return 0;
    }

    struct list_head *pos;
    unsigned long nearest = now;
    unsigned char firstSet = FALSE;

    list_for_each(pos,&timerListRunning) {
        mytimer_t *timer = (mytimer_t *)list_entry(pos, mytimer_t, list);

        if (firstSet) {
            if (time_after(nearest, timer->expire)) {
                nearest = timer->expire;
            }
        }
        else {
            firstSet= TRUE;
            nearest = timer->expire;
        }
    }

    return nearest;
}

//only using in timerThread()
static unsigned int reSchedule()
{
    unsigned long nextTimeout, now;

    now = getCurrentTime(NULL);

    timer_lock(tCb.semid);
    checkTimerList(now);
    nextTimeout = checkNearestTimeout(now);
    timer_unlock(tCb.semid);

    if (nextTimeout == 0)
        goto exit;

    if (now < nextTimeout) {
        nextTimeout = (nextTimeout - now);
    }
    else {
        nextTimeout = (nextTimeout+~now);
    }

exit:
    return nextTimeout;
}

static void launchProcessTimerListThread()
{
    pthread_t a_thread;
    pthread_attr_t a_thread_attr;
    PTHREAD_SET_UNATTACH_ATTR(a_thread_attr);

    //pthread_attr_setstacksize(&a_thread_attr, 512*1024);

    if(pthread_create(&a_thread, &a_thread_attr, processTimerListThread, NULL)!=0) {
        BUG_ON();
        return;
    }

    pthread_attr_destroy(&a_thread_attr);
}

#if TIMERDEBUG
static unsigned int listLen(struct list_head *head)
{
    struct list_head *pos;
    unsigned int count = 0;

    if (list_empty(head)) {
        return count;
    }

    list_for_each(pos,head) {
        count++;
    }

    return count;
}

static void printCounter()
{
    stdprintf("timerListRunning [%u]\n",listLen(&timerListRunning));
    stdprintf("timerListExpired [%u]\n",listLen(&timerListExpired));
    stdprintf("timerListExec    [%u]\n",listLen(&timerListExec));
}
#endif

static void *timerThread(void *arg)
{
    logger (LOG_INFO, "Thread Create[timerThread]");

    if (!(pthread_setcancelstate(PTHREAD_CANCEL_ENABLE ,NULL)==0 && pthread_setcanceltype(PTHREAD_CANCEL_ASYNCHRONOUS ,NULL) == 0)) {
        BUG_ON();
        pthread_exit(0);
    }

    logger (LOG_INFO, "Timer Thread Running");

    struct timeval timeout;
    timeout.tv_sec = 0;
    timeout.tv_usec = 0;
    int ret;

    for(;;) {
        fd_set lfdset;
        FD_ZERO( &lfdset );
        FD_SET(socket_pair[0], &lfdset);

#if TIMERDEBUG
        stdprintf("[For Loop] next wake up after %d seconds\n", (int )timeout.tv_sec);
#endif

        if (timeout.tv_sec > 0) {
            //stdprintf("Wait Select w/ %d seconds timeout\n", (int )timeout.tv_sec);
            ret = select( socket_pair[0] + 1, &lfdset, (fd_set*) 0, (fd_set*) 0, (struct timeval*)&timeout);
            //stdprintf("Exit Select\n");
        }
        else {
            //stdprintf("Wait Select until signal\n");
            ret = select( socket_pair[0] + 1, &lfdset, (fd_set*) 0, (fd_set*) 0, NULL);
            //stdprintf("Exit Select\n");
        }

        if ( ret < 0 ) {
            if ( errno == EINTR || errno == EAGAIN ) {
                continue;
            }
            perror("Timer Select:");
            BUG_ON();
        }

        if (ret == 0) {
            timeout.tv_sec = reSchedule();
#if TIMERDEBUG
            stdprintf("[Timeout ] next wake up after %d seconds\n", (int )timeout.tv_sec);
#endif
            launchProcessTimerListThread();
        }
        else {
            if (FD_ISSET(socket_pair[0], &lfdset)) {
                int sig;
                read(socket_pair[0], &sig, sizeof(sig));    //don't care what we read
                timeout.tv_sec = reSchedule();
#if TIMERDEBUG
                stdprintf("[Signal  ] next wake up after %d seconds\n", (int )timeout.tv_sec);
#endif
                launchProcessTimerListThread();
            }
        }

#if TIMERDEBUG
        printCounter();
#endif
    }

    pthread_exit(0);
}

/**
 *  @brief Create timer thread. Only one timer thread can be create in the one process.
 *  @param *cb Must give a valid semaphore id. 
 *
 *  @return TRUE for success and FALSE for failed.
 */
unsigned char createTimerThread(tThreadCb *cb)
{
    if (timerThreadAlreadyLaunch)   //Fix Me: racing issue!?
        return FALSE;

    memcpy(&tCb, cb, sizeof(tThreadCb));

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, socket_pair)!=0) {
        stdprintf("can't create signal pipe\n");
        return FALSE;
    }

    pthread_t a_thread;
    pthread_attr_t a_thread_attr;
    PTHREAD_SET_UNATTACH_ATTR(a_thread_attr);

    if(pthread_create(&a_thread, &a_thread_attr, timerThread, (void *)&tCb)!=0) {
        BUG_ON();
        return FALSE;
    }

    pthread_attr_destroy(&a_thread_attr);
    timerThreadAlreadyLaunch = TRUE;

    tCb.id = a_thread;

    return TRUE;
}

static void freeTimerList(struct list_head *head)
{
    if (list_empty(head))
        return;

    struct list_head *pos, *n;

    list_for_each_safe(pos,n,head) {
        list_move(pos,&timerListExpired);
    }
}


/**
 *  @brief Destroy timer thread
 *
 *  @return None.
 */
void destoryTimerThread()
{
    if (timerThreadAlreadyLaunch) {
        timer_lock(tCb.semid);
        //cancel timer thread
        pthread_cancel(tCb.id);
        //lock list and move timerListRunning and timerListExec to timerListExpired
        freeTimerList(&timerListRunning);
        freeTimerList(&timerListExec);

        //release socketpair
        close(socket_pair[0]);
        close(socket_pair[1]);
        timer_unlock(tCb.semid);
    }

    return;
}

/**
 *  @brief Add timer
 *  @param *timer Timer user defined.
 *  @return None.
 */
void addTimer(mytimer_t *timer)
{
    timer_lock(tCb.semid);
    list_add_tail(&timer->list,&timerListRunning);
    sendNotifyToTimerThread(timer_addTimer);
    timer_unlock(tCb.semid);
}

void __delTimer(mytimer_t *timer)
{
    list_del(&timer->list);

    sendNotifyToTimerThread(timer_delTimer);
}

/**
 *  @brief Delete timer
 *  @param *timer Timer you want to delete. 
 *  @return None.
 */
void delTimer(mytimer_t *timer)
{
    timer_lock(tCb.semid);
    __delTimer(timer);
    timer->list.next = timer->list.prev = &timer->list;
    timer_unlock(tCb.semid);
}

void __modTimer(mytimer_t *timer, unsigned int expire)
{
    list_del(&timer->list);
    timer->expire = expire;
    list_add_tail(&timer->list,&timerListRunning);

    sendNotifyToTimerThread(timer_modTimer);
}

/**
 *  @brief Modify timer
 *  @param *timer Timer you want to delete. 
 *  @param expire The new expire you want add to timer.
 *  @return None.
 */
void modTimer(mytimer_t *timer, unsigned int expire)
{
    timer_lock(tCb.semid);
    __modTimer(timer,expire);
    timer_unlock(tCb.semid);
}


