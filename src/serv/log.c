#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

static int start(void)
{
    char enable[64];
    cdb_get("$log_rm_enable", enable);

    if (!strcmp(enable, "0") || !strcmp(enable, "!ERR")) {
        return 0;
    }
    if (f_exists("/sbin/syslogd")) {
        char sys_ip[64];
        char cmd[128];
        cdb_get("$log_sys_ip", sys_ip);
        if ((sys_ip[0] == '\0') || !strcmp(enable, "!ERR")) {
            sprintf(cmd, "syslogd -s 1");
        }
        else {
            sprintf(cmd, "syslogd -s 1 -L -R %s:514", sys_ip);
        }
        exec_cmd(cmd);
    }
    return 0;
}

static int stop(void)
{
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop },
    { .cmd = NULL    }
};

int log_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

