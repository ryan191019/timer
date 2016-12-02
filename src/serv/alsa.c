#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

static int start(void)
{
    exec_cmd("alsactl init");

    if(f_exists("/PowerOn.wav")) {
        MTC_LOGGER(this, LOG_INFO, "Playing PowerOn.wav");
        eval_nowait("aplay", "/PowerOn.wav");
    }
    else {
        MTC_LOGGER(this, LOG_INFO, "There is no PowerOn.wav");
    }

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int alsa_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

