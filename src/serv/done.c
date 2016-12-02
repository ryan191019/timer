#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

static int start(void)
{
    char cpuclk[4] = {0};
    char sysclk[4] = {0};

    if (f_isdir("/tmp/root")) {
        exec_cmd("lock /tmp/.switch2jffs; firstboot switch2jffs; lock -u /tmp/.switch2jffs");
        exec_cmd("touch /tmp/root/done");
        if (f_exists("/etc/init.d/usb")) {
            exec_cmd("/etc/init.d/usb start");
        }
    }
    if (f_exists("/etc/rc.local")) {
        exec_cmd("/etc/rc.local");
    }

    cdb_get("$chip_cpu", cpuclk);
    cdb_get("$chip_sys", sysclk);
    exec_cmd2("echo %s %s > /proc/soc/clk", cpuclk, sysclk);
    exec_cmd("echo sclk > /proc/eth");
    exec_cmd("echo an > /proc/eth");

	if (get_pid_by_name("monitor") < 0) {
		exec_cmd("monitor &");
	}
    exec_cmd("echo 1 > /tmp/boot_done");

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int done_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

