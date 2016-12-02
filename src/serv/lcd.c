#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

static int start(void)
{
    if(f_exists("/etc/_volume"))
        exec_cmd("cp /etc/_volume /etc/hotplug.d/volume/action");

    if(f_exists("/proc/pwm")) {
        exec_cmd("echo \"set A\" > /proc/pwm");
        exec_cmd("echo \"pol 1\" > /proc/pwm");
        exec_cmd("echo \"mode 0\" > /proc/pwm");
        exec_cmd("echo \"duty 0\" > /proc/pwm");
        exec_cmd("echo \"en 1\" > /proc/pwm");
    }
    eval_nowait("ev", "0");
    eval("mcc");
    return 0;
}

static int stop(void)
{
    exec_cmd("killall -s 9 mcc");
    exec_cmd("killall -s 9 ev");
    
    if(f_exists("/proc/pwm")) {
        exec_cmd("echo \"set A\" > /proc/pwm");
        exec_cmd("echo \"en 0\" > /proc/pwm");
    }
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int lcd_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

