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
    mkdir("/var/lib", 0755);
		mkdir("/var/lib/dbus", 0755);
		mkdir("/var/run/dbus", 0755);

    if(!f_exists("/var/lib/dbus/machine-id"))
        exec_cmd("/usr/bin/dbus-uuidgen --ensure");

    exec_cmd("/usr/sbin/dbus-daemon --system");
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int dbus_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

