#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_misc.h"
#include "include/mtc_proc.h"

extern MtcData *mtc;

char mac[16];
char ipaddr[64];
char sw_vid[64];
char sw_pid[64];
int omi_result;
char version_time[64];

static void get_parm(void)
{
    char t_mac[32];
    FILE *fp;
    
    boot_cdb_get("mac0", t_mac);
    sprintf(mac, "%c%c%c%c%c%c%c%c%c%c%c%c%c", t_mac[0], t_mac[1], t_mac[3], t_mac[4], t_mac[6], t_mac[7], t_mac[9], t_mac[10], t_mac[12], t_mac[13], t_mac[15], t_mac[16], '\0');

    cdb_get_str("$lan_ip", ipaddr, sizeof(ipaddr), NULL);
    cdb_get_str("$sw_vid", sw_vid, sizeof(sw_vid), NULL);
    cdb_get_str("$sw_pid", sw_pid, sizeof(sw_pid), NULL);
    omi_result = cdb_get_int("$omi_result", 0);

    if((fp = _system_read("cat /proc/uptime | cut -d '.' -f1"))) {
        new_fgets(version_time, sizeof(version_time), fp);
        _system_close(fp);
    }

    if((mtc->rule.OMODE == 8) || (mtc->rule.OMODE == 9))
        strcpy(ipaddr, "");
}

int mtc_ocfg_arg(void)
{
    char cmd[512];
    
    get_parm();
    
    sprintf(cmd, "'start-stop-daemon -S -x omnicfg_broadcast \"{\\\"ver\\\":\\\"1\\\",\\\"name\\\":\\\"OMNICFG@%s\\\",\\\"omi_result\\\":\\\"%d\\\",\\\"sw_vid\\\":\\\"%s\\\",\\\"sw_pid\\\":\\\"%s\\\",\\\"lan_ip\\\":\\\"%s\\\",\\\"version\\\":\\\"%s\\\",\\\"new_conf\\\":\\\"1\\\"}\" \"{\\\"ver\\\":\\\"1\\\",\\\"name\\\":\\\"OMNICFG@%s\\\",\\\"omi_result\\\":\\\"%d\\\",\\\"sw_vid\\\":\\\"%s\\\",\\\"sw_pid\\\":\\\"%s\\\",\\\"lan_ip\\\":\\\"%s\\\",\\\"version\\\":\\\"%s\\\",\\\"new_conf\\\":\\\"0\\\"}\" 60 -b'", mac, omi_result, sw_vid, sw_pid, ipaddr, version_time, mac, omi_result, sw_vid, sw_pid, ipaddr, version_time);

    exec_cmd2("eval %s", cmd);
    //exec_cmd2("logger ocfgarg--\"%s\"", cmd);
    
    return 0;
}

static int start(void)
{
    char cmd[512];

    get_parm();

    sprintf(cmd, "'start-stop-daemon -S -x omnicfg_broadcast \"{\\\"ver\\\":\\\"1\\\",\\\"name\\\":\\\"OMNICFG@%s\\\",\\\"omi_result\\\":\\\"%d\\\",\\\"sw_vid\\\":\\\"%s\\\",\\\"sw_pid\\\":\\\"%s\\\",\\\"lan_ip\\\":\\\"%s\\\",\\\"version\\\":\\\"%s\\\",\\\"new_conf\\\":\\\"0\\\"}\" \"{\\\"ver\\\":\\\"1\\\",\\\"name\\\":\\\"OMNICFG@%s\\\",\\\"omi_result\\\":\\\"%d\\\",\\\"sw_vid\\\":\\\"%s\\\",\\\"sw_pid\\\":\\\"%s\\\",\\\"lan_ip\\\":\\\"%s\\\",\\\"version\\\":\\\"%s\\\",\\\"new_conf\\\":\\\"0\\\"}\" 60 -b'", mac, omi_result, sw_vid, sw_pid, ipaddr, version_time, mac, omi_result, sw_vid, sw_pid, ipaddr, version_time);

    exec_cmd2("eval %s", cmd);
    //exec_cmd2("logger ocfg start--\"%s\"", cmd);

    return 0;
}

static int stop(void)
{
    exec_cmd("killall -SIGHUP omnicfg_broadcast");
    //exec_cmd("logger ocfg stop");
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int ocfg_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

