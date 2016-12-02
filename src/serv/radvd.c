#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <netinet/in.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

extern MtcData *mtc;

#define RADVD_NAME       "radvd"
#define RADVD_CMDS       "/usr/sbin/"RADVD_NAME
#define RADVD_ARGS       "-m stderr_syslog"
#define RADVD_CONFILE    "/tmp/"RADVD_NAME".conf"
#define RADVD_PIDFILE    "/var/run/"RADVD_NAME".pid"

static int start(void)
{
/*
$lan6_autocfg=2
$lan6_end=200
$lan6_gw=2008:1::1
$lan6_ip=2001:0db8:3c4d:0015::1
$lan6_mode=2
$lan6_msk=2008:1::1
$lan6_prfxlen=64
$lan6_start=100
$lan6_ttl=600
*/
    FILE *fp;
    char lan6_ip[INET6_ADDRSTRLEN] = { 0 };
    int lan6_prfxlen = cdb_get_int("$lan6_prfxlen", 64);
    int lan6_ttl = cdb_get_int("$lan6_ttl", 600);
    // max=600 sec, min=0.33*max sec
    int max = 600;
    int min = 200;

    if ((cdb_get_str("$lan6_ip", lan6_ip, sizeof(lan6_ip), NULL) == NULL) ||
        (!strcmp(lan6_ip, ""))){
        MTC_LOGGER(this, LOG_ERR, "load args fail, lan6_ip=[%s]", lan6_ip);
        return 0;
    }

    fp = fopen(RADVD_CONFILE, "w");
    if (fp) {
        fprintf(fp, "interface %s\n", mtc->rule.LAN);
        fprintf(fp, "{\n");
        fprintf(fp, "   AdvSendAdvert on;\n");
        fprintf(fp, "   MaxRtrAdvInterval %d;\n", max);
        fprintf(fp, "   MinRtrAdvInterval %d;\n", min);
        fprintf(fp, "   prefix %s/%d\n", lan6_ip, lan6_prfxlen);
        fprintf(fp, "   {\n");
        fprintf(fp, "       AdvOnLink on;\n");
        fprintf(fp, "       AdvAutonomous on;\n");
        fprintf(fp, "       AdvPreferredLifetime %d;\n", lan6_ttl);
        fprintf(fp, "       AdvValidLifetime %d;\n", lan6_ttl);
        fprintf(fp, "   };\n");
        fprintf(fp, "};\n");
        fclose(fp);
    }

    exec_cmd2("ifconfig %s %s", mtc->rule.LAN, lan6_ip);
    exec_cmd2("sysctl -w net.ipv6.conf.all.forwarding=1 > /dev/null 2> /dev/null");
    exec_cmd2("start-stop-daemon -S -b -p %s -x %s -- %s -C %s", RADVD_PIDFILE, RADVD_CMDS, RADVD_ARGS, RADVD_CONFILE);

    return 0;
}

static int stop(void)
{
    if( f_exists(RADVD_PIDFILE) ) {
        exec_cmd2("start-stop-daemon -K -q -p %s", RADVD_PIDFILE);
        unlink(RADVD_PIDFILE);
    }
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int radvd_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

