
/**
 * @file mtc_time.c
 *
 * @author  Frank Wang
 * @date    2015-10-15
 */

#include <string.h>

#include "include/mtc.h"
#include "include/mtc_misc.h"
#include "include/mtc_timer.h"
#include "include/mtc_time.h"

extern int semId[MAX_SEM_LIST];

static mytimer_t fixTimerList[timerMax];

#define DEBUGTIMER  0
#define TIMERSAMPLE 0

#if TIMERSAMPLE
static int testNum = 0;
void timerTestFunc(int sData, void *data)
{
    int *private = data;

    stdprintf("Time = %lu, sData = %d, data = %d\n",getCurrentTime(NULL), sData++, *private);

    *private+=1;

    if (*private<100)
        mtcTimerMod(timerTest,3);   //not need to lock when call back func running.
    else
        mtcTimerDel(timerTest);
}
#endif

void mtcTimerInit()
{
    tThreadCb t;
    int i;

    t.semid = mtc_get_semid(semTimer);

    if(t.semid == UNKNOWN) {
        stdprintf("semid is not available\n");
        return;
    }

    memset(fixTimerList, 0, timerMax * sizeof(mytimer_t));

    for(i=0; i<timerMax; i++) {
        fixTimerList[i].list.next = fixTimerList[i].list.prev = &fixTimerList[i].list;
    }

    if (!createTimerThread(&t)) {
        stdprintf("Timer System Start Fail\n");
        return;
    }

#if TIMERSAMPLE
    mtcTimerAdd(timerTest, timerTestFunc, 0, &testNum, 3);
#endif
}

void mtcTimerDel(fixTimerList_t id)
{
#if DEBUGTIMER
    stdprintf("[mtcTimerDel][%d][S]\n",id);
#endif
    delTimer(&fixTimerList[id]);
#if DEBUGTIMER
    stdprintf("[mtcTimerDel][%d][E]\n",id);
#endif

}

void mtcTimerAdd(fixTimerList_t id, timer_func func, int sData, void *data, unsigned int second)
{
    unsigned int now;

    if (!func) {
        BUG_ON();
        return;
    }
#if DEBUGTIMER
    else {
        stdprintf("id %d at %p\n", id, func);
    }
#endif

    mtcTimerDel(id);

    fixTimerList[id].func = func;
    fixTimerList[id].sData = sData;
    if (fixTimerList[id].data)
        free(fixTimerList[id].data);
    fixTimerList[id].data = data;

    now = getCurrentTime(NULL);

    fixTimerList[id].expire = now + second;

#if DEBUGTIMER
    stdprintf("[mtcTimerAdd][%d][%u][%u][%lu][S]\n",id, now, second, fixTimerList[id].expire);
#endif

    addTimer(&fixTimerList[id]);

#if DEBUGTIMER
    stdprintf("[mtcTimerAdd][%d][%u][%u][%lu][E]\n",id, now, second, fixTimerList[id].expire);
#endif
}

void mtcTimerMod(fixTimerList_t id, unsigned int second)
{
    unsigned int now = getCurrentTime(NULL);

#if DEBUGTIMER
    stdprintf("[mtcTimerMod][%d][%u][%u][%lu][S]\n",id, now, second, fixTimerList[id].expire);
#endif
    modTimer(&fixTimerList[id], now + second);
#if DEBUGTIMER
    stdprintf("[mtcTimerMod][%d][%u][%u][%lu][E]\n",id, now, second, fixTimerList[id].expire);
#endif

}

