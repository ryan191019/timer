#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/mount.h>
#include <sys/types.h> 
#include <sys/stat.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"
#include "misc.h"

static int start(void)
{
    FILE *fp;
    char root[128];

	exec_cmd("setLanguage.sh init");
	exec_cmd("aplay /tmp/res/starting_up.wav");
    if(f_exists("/proc/mounts"))
        exec_cmd("/sbin/mount_root");

    if(f_exists("/proc/jffs2_bbc"))
       f_write_string("/proc/jffs2_bbc", "\"S\"\n" , 0, 0);

    if(f_exists("/proc/net/vlan/config"))
        exec_cmd("vconfig set_name_type DEV_PLUS_VID_NO_PAD");

    mkdir("/var/log", 0755);
		mkdir("/var/lock", 0755);
		mkdir("/var/state", 0755);
		mkdir("/var/.uci", 0700);
    
    exec_cmd("touch /var/log/wtmp");
    exec_cmd("touch /var/log/lastlog");
    exec_cmd("touch /tmp/resolv.conf.auto");
    exec_cmd("ln -sf /tmp/resolv.conf.auto /tmp/resolv.conf");

		mount("debugfs", "/sys/kernel/debug", "debugfs", MS_MGC_VAL, NULL);

    if(f_exists("/lib/wdk/system")) {
        system_update_hosts();
        exec_cmd("/lib/wdk/system timezone");
    }

    if((fp = _system_read("awk 'BEGIN { RS=\" \"; FS=\"=\"; } $1 == \"root\" { print $2 }' < /proc/cmdline"))) {
        new_fgets(root, sizeof(root), fp);
        eval("ln", "-s", root, "/dev/root");
        _system_close(fp);
    }

    //exec_cmd("insmod fuse");

    if(f_exists("/usr/sbin/reset_btn")) {
        exec_cmd("killall reset_btn");
        exec_cmd("/usr/sbin/reset_btn");
    }

    MTC_LOGGER(this, LOG_INFO, "boot done");

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { .cmd = NULL    }
};

int boot_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

