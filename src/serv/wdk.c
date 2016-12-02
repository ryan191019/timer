#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

/* 
* format: BTN_x $gpio $low_trig $type "${desc}" 
*/
static int init_all_gpio(void)
{
	 /* BTN_x $gpio $low_trig $type "${desc}" */
	 f_write_string("/proc/cta-gpio-buttons", "stop\n", 0, 0);
	 f_write_string("/proc/cta-gpio-buttons", "set 0 6 1 1 \"reboot/reset\"\n", 0, 0);
	 f_write_string("/sys/class/gpio/export","6\n", 0, 0);
	 f_write_string("/sys/class/gpio/gpio6/direction", "in\n", 0, 0);
	 
	 f_write_string("/proc/cta-gpio-buttons", "set 1 12 1 1 \"wps\"\n", 0, 0);
	 f_write_string("/sys/class/gpio/export","12\n", 0, 0);
	 f_write_string("/sys/class/gpio/gpio12/direction", "in\n", 0, 0);
	 
	 f_write_string("/proc/cta-gpio-buttons", "set 2 8 1 1 \"switch\"\n", 0, 0);
	 f_write_string("/sys/class/gpio/export","8\n", 0, 0);
	 f_write_string("/sys/class/gpio/gpio8/direction", "in\n", 0, 0);
	 
	 f_write_string("/proc/cta-gpio-buttons", "start\n", 0, 0);
	 
	 
	 return 0;
}

/*
* initializing all madc
* format: set $chan $type
*/
static int init_all_madc(void)
{
	 f_write_string("/proc/madc", "total 2\n", 0, 0);
	 f_write_string("/proc/madc", "stop\n", 0, 0);
	 f_write_string("/proc/madc", "reset\n", 0, 0);
	 /* set $chan $type */
	 f_write_string("/proc/madc", "set 0 2\n", 0, 0);
	 f_write_string("/proc/madc", "set 1 2\n", 0, 0);
	 f_write_string("/proc/madc", "start\n", 0, 0);
	 return 0;
}

static int start(void)
{
    char codec[BUF_SIZE];

  	f_write_string("/proc/eth", "sclk\n", 0, 0);
		f_write_string("/proc/eth", "an\n", 0, 0);

    if (setup_codec_info(codec, sizeof(codec)) == EXIT_SUCCESS)
        cdb_set("$mpd_codec_info", codec);

    init_all_gpio();
    init_all_madc();

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int wdk_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

