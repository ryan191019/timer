
/**
 * @file mtc_fsm.c
 *
 * @author  Frank Wang
 * @date    2015-10-06
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <errno.h>

#include "include/mtc_ipc.h"
#include "include/mtc_misc.h"
#include "include/mtc.h"

#include "include/mtc_fsm.h"
#include "include/mtc_time.h"

static int fsm_pipe[2];

static fsm_private_t fsm_data;

fsm_mode mtc_get_fsm_mode()
{
    mtc_lock(semFSMData);
    fsm_mode ret = fsm_data.mode;
    mtc_unlock(semFSMData);

    return ret;
}

void mtc_set_fsm_mode(fsm_mode mode)
{
    mtc_lock(semFSMData);
    fsm_data.mode = mode;
    mtc_unlock(semFSMData);
}

static void mtc_fsm_process(unsigned char isTimeout)
{
    if(fsm_data.mode == fsm_mode_stop)
        return;

    mtc_lock(semFSMData);
    if(isTimeout) {
        if(fsm_data.mode != fsm_mode_idle)
            fsm_data.mode = fsm_mode_idle;
    }
    else {
        if(fsm_data.mode != fsm_mode_running) {
            fsm_data.mode = fsm_mode_running;
        }
    }
    mtc_unlock(semFSMData);
}

static void mtc_fsm_init()
{
    memset(&fsm_data, 0, sizeof(fsm_private_t));
    fsm_data.mode = fsm_mode_running;

    return;
}

void mtc_fsm_schedule(fsm_event event)
{
    int reSendCount = 0;

reSend:
    if (reSendCount++>5) {
        stdprintf("Can't Send Event to Schedule\n");
        return;
    }

    if (send(fsm_pipe[1], &event, sizeof(event), MSG_DONTWAIT) < 0) {
        if ( errno == EINTR || errno == EAGAIN ) {
            goto reSend;    /* try again */
        }
        stdprintf("Could not send event: %d\n", event);
    }
}

void mtc_fsm_boot()
{
    return;
}

void mtc_fsm_stop()
{
    mtc_set_fsm_mode(fsm_mode_stop);
    return;
}

void mtc_fsm()
{
    mtc_fsm_init();

    fd_set lfdset;

    if (socketpair(AF_UNIX, SOCK_STREAM, 0, fsm_pipe)!=0) {
        BUG_ON();
        return;
    }

    while(fsm_data.mode != fsm_mode_stop) {
reSelect:
        FD_ZERO( &lfdset );
        FD_SET(fsm_pipe[0], &lfdset);

        struct timeval timeout;
        timeout.tv_sec = 3;
        timeout.tv_usec = 0;

        int ret = select(fsm_pipe[0] + 1, &lfdset, (fd_set*) 0, (fd_set*) 0, (struct timeval*)&timeout);

        if (ret < 0) {
            if(errno == EINTR || errno == EAGAIN) {
                goto reSelect;    /* try again */
            }
        }

        if (ret == 0) {
            mtc_fsm_process(TRUE);
            continue;
        }

        fsm_event sig[BUF_LEN_16];
        memset(sig,0,sizeof(sig));
        int getLen=read(fsm_pipe[0], sig, sizeof(sig));

        if (getLen < 0) {
            continue;
        }

        int i;
        for (i=0;i<getLen/sizeof(fsm_event);i++) {
            if (sig[i]==fsm_boot) {
                mtc_fsm_boot();
                goto reSelect;
            }
            else if (sig[i]==fsm_stop) {
                mtc_fsm_stop();
                break;
            }
        }
        mtc_fsm_process(FALSE);
    }

    return;
}

