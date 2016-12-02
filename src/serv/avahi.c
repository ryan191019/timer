#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

static int start(void)
{
    exec_cmd("sysctl -w net.ipv4.conf.default.force_igmp_version=2");
    mkdir("/var/run/avahi-daemon", 0755);
    
    exec_cmd("/usr/sbin/avahi-daemon -D");
    MTC_LOGGER(this, LOG_INFO, "avahi-daemon is running");
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int avahi_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

