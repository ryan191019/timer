#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

static int start(void)
{
    return 0;
}

static int stop(void)
{
    sync();
    exec_cmd("umount -a -r");

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop },
    { .cmd = NULL    }
};

int umount_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

