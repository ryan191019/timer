#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

extern MtcData *mtc;

static int do_set_arp_lan_bind(void)
{
    char sbuf[USBUF_LEN];
    char entry[MSBUF_LEN];
	char *argv[10];
    char *ip, *mac;
	int num, i, en;

    en = cdb_get_int("$lan_arp_sp", 0);

    if (en != 0) {
        for (i=0;i<20;i++) {
            sprintf(sbuf, "$lan_arp_entry%d", i);
            if (cdb_get_str(sbuf, entry, sizeof(entry), NULL) == NULL)
                break;
            num = str2argv(entry, argv, '&');
            if(num < 4)
                break;
            if(!str_arg(argv, "en=")||!str_arg(argv, "host=")||!str_arg(argv, "ip=")||!str_arg(argv, "mac="))
                continue;
            en = atoi(str_arg(argv, "en="));
            if (en) {
                mac = str_arg(argv, "mac=");
                ip = str_arg(argv, "ip=");
                exec_cmd2("arp -s %s %s > /dev/null 2>&1", ip, mac);
            }
        }
    }

    return 0;
}

static int do_clear_arp_perm(void)
{
    char cmd[MAX_COMMAND_LEN];
    FILE *fp;

    sprintf(cmd, "arp -na | awk '/PERM on %s/{gsub(/[()]/,\"\",$2); print $2 }'", mtc->rule.LAN);
    if((fp = _system_read(cmd))) {
        while ( new_fgets(cmd, sizeof(cmd), fp) != NULL) {
            exec_cmd2("arp -d %s > /dev/null 2>&1", cmd);
        }
        _system_close(fp);
    }

    return 0;
}

static int start(void)
{
    char ipaddr[PBUF_LEN];
    char ntmask[PBUF_LEN];

    cdb_get("$lan_ip", ipaddr);
    cdb_get("$lan_msk", ntmask);

    exec_cmd2("ifconfig lo 127.0.0.1 netmask 255.0.0.0 up > /dev/null 2>&1");
    if ((mtc->rule.OMODE != opMode_p2p) && (mtc->rule.OMODE != opMode_client)) {
        exec_cmd2("ifconfig eth0 up > /dev/null 2>&1");
    }
    exec_cmd2("ifconfig %s %s netmask %s up > /dev/null 2>&1", mtc->rule.LAN, ipaddr, ntmask);

    do_set_arp_lan_bind();

    return 0;
}

static int stop(void)
{
    do_clear_arp_perm();
    exec_cmd2("ifconfig lo 0.0.0.0 down > /dev/null 2>&1");
    exec_cmd2("ifconfig %s 0.0.0.0 down > /dev/null 2>&1", mtc->rule.LAN);

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int lan_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

