#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc_linklist.h"
#include "include/mtc.h"
#include "include/mtc_proc.h"


#define cprintf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt , ## args); \
                fclose(fp); \
        } \
} while (0)

static int start(void)
{
#if 0
    exec_cmd("/etc/init.d/uartd start > /dev/null ");
#else
	cprintf("%s:%d *********** uartd start...",__func__,__LINE__);
	exec_cmd("rm /etc/config/snapcastServer.json");
	exec_cmd("rm /tmp/UartFIFO");
	if(!f_exists("/dev/gpio"))
        exec_cmd("mknod /dev/gpio c 252 0");
	exec_cmd("uartd &");
	exec_cmd("/usr/bin/elian.sh check &");
	cprintf("%s:%d *********** uartd start end...",__func__,__LINE__);
#endif
    return 0;
}

static int stop(void)
{
    exec_cmd("killall -9 uartd");
    return 0;
}


static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop", stop},
    { .cmd = NULL    }
};

int uartd_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

