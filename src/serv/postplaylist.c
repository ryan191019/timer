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
    exec_cmd("/etc/init.d/postplaylist start > /dev/null ");
#else
	cprintf("%s:%d *********** postplaylist start...",__func__,__LINE__);

	exec_cmd("WIFIAudio_DownloadBurn &  ");
	cprintf("%s:%d *********** postplaylist start end...",__func__,__LINE__);
#endif
    return 0;
}

static int stop(void)
{

	exec_cmd("killall  -9 WIFIAudio_DownloadBurn");
    return 0;
}


static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  }, 
    { .cmd = NULL    }
};

int postplaylist_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

