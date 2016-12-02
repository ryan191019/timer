
/**
 * @file mtc_time.h
 *
 * @author  Frank Wang
 * @date    2015-10-15
 */

#ifndef _MTC_TIME_H_ 
#define _MTC_TIME_H_ 

#include "mtc_timer.h"

typedef enum
{
    timerTest = 0,
    timerMtcStartBoot,
    timerMtcBrProbe,

    timerMax
}fixTimerList_t;

typedef struct
{
    timer_func  func;
    int         sData;
    void        *data;
}mtctimer_t;

void mtcTimerInit();

void mtcTimerAdd(fixTimerList_t id, timer_func func, int sData, void *data, unsigned int expire);
void mtcTimerDel(fixTimerList_t id);
void mtcTimerMod(fixTimerList_t id, unsigned expire);


#endif

