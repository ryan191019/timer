#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

static int start(void)
{
    //unlink("/etc/crontabs/root");
    //exec_cmd("echo \"# m h dom mon dow command\" > /etc/crontabs/root");
    //exec_cmd("crond");
    exec_cmd("crond");
		cprintf("%s:%d start crond\n", __func__, __LINE__);
    return 0;
}
static int stop()
{
	//exec_cmd("rm -f /etc/crontabs/root");
	//exec_cmd("echo \"# m h dom mon dow command\" > /etc/crontabs/root");
	exec_cmd("killall -9 crond");
	cprintf("%s:%d killall -9 crond\n", __func__, __LINE__);
	return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    {"stop", 	stop},
    { .cmd = NULL    }
};

int crond_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

