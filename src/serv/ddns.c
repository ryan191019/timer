#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

extern MtcData *mtc;

#if defined(CONFIG_PACKAGE_inadyn)
#define DDNS_NAME       "inadyn"
#define DDNS_CMDS       "/usr/sbin/"DDNS_NAME
#define DDNS_ARGS       "--input_file /tmp/"DDNS_NAME".conf"
#define DDNS_CONFILE    "/tmp/"DDNS_NAME".conf"
#define DDNS_PIDFILE    "/var/run/"DDNS_NAME".pid"
#define DDNS_LOGFILE    "/var/run/"DDNS_NAME".log"
#define DDNS_CACHE      "/tmp/"DDNS_NAME".cache"
#define DDNS_SERVICE    "dyndns"
#elif defined(CONFIG_PACKAGE_ez_ipupdate)
#define DDNS_NAME       "ez-ipupdate"
#define DDNS_CMDS       "/usr/sbin/"DDNS_NAME
#define DDNS_ARGS       "-c /tmp/"DDNS_NAME".conf -d"
#define DDNS_CONFILE    "/tmp/"DDNS_NAME".conf"
#define DDNS_PIDFILE    "/var/run/"DDNS_NAME".pid"
#define DDNS_LOGFILE    "/var/run/"DDNS_NAME".log"
#define DDNS_CACHE      "/tmp/"DDNS_NAME".cache"
#define DDNS_SERVICE    "dyndns"
#else
#define DDNS_NAME       ""
#define DDNS_CMDS       ""
#define DDNS_ARGS       ""
#define DDNS_CONFILE    ""
#define DDNS_PIDFILE    ""
#define DDNS_LOGFILE    ""
#define DDNS_CACHE      ""
#define DDNS_SERVICE    ""
#endif

static int FUNCTION_IS_NOT_USED ddns_stop(void)
{
    if( f_exists(DDNS_PIDFILE) ) {
        exec_cmd2("start-stop-daemon -K -q -s 9 -p %s", DDNS_PIDFILE);
        unlink(DDNS_PIDFILE);
    }
    return 0;
}

static int FUNCTION_IS_NOT_USED ddns_start(void)
{
//#$ddns1=un=user&pw=pass&retry=3600&en=1&srv=dyndns&hn=test2.dyndns.net
//#$ddns_enable=1

    FILE *fp;
    char ddns1[MSBUF_LEN];
    char server[SSBUF_LEN] = { 0 };
    char uri[SSBUF_LEN] = { 0 };
    char *argv[10];
    char *un, *pw, *srv, *hn;
    int num, retry, en;
    int ddns_enable = cdb_get_int("$ddns_enable", 0);

    if (!ddns_enable) {
        MTC_LOGGER(this, LOG_INFO, "ddns_enable=0");
        return 0;
    }

    if (cdb_get_str("$ddns1", ddns1, sizeof(ddns1), NULL) == NULL)
        goto err_args;
    num = str2argv(ddns1, argv, '&');
    if(num < 6)
        goto err_args;
    if(!str_arg(argv, "un=")    || !str_arg(argv, "pw=") ||
       !str_arg(argv, "retry=") || !str_arg(argv, "en=") ||
       !str_arg(argv, "srv=")   || !str_arg(argv, "hn="))
        goto err_args;
    retry = atoi(str_arg(argv, "retry="));
    en = atoi(str_arg(argv, "en="));
    if (en) {
        un = str_arg(argv, "un=");
        pw = str_arg(argv, "pw=");
        srv = str_arg(argv, "srv=");
        hn = str_arg(argv, "hn=");

#if defined(CONFIG_PACKAGE_inadyn)
        (void) uri;
        if (!strcmp(srv, "dyndns")) {
            snprintf(server, SSBUF_LEN, "dyndns@dyndns.org");
        }
        else if (!strcmp(srv, "noip")) {
            snprintf(server, SSBUF_LEN, "default@no-ip.com");
        }
        else if (!strcmp(srv, "tzo")) {
            snprintf(server, SSBUF_LEN, "default@tzo.com");
        }

        fp = fopen(DDNS_CONFILE, "w");
        if (fp) {
            fprintf(fp, "update_period_sec 60000\n");
            fprintf(fp, "username %s\n", un);
            fprintf(fp, "password %s\n", pw);
            fprintf(fp, "dyndns_system %s\n", server);
            fprintf(fp, "alias %s\n", hn);
            fprintf(fp, "iterations %d\n", retry);
            fprintf(fp, "log_file %s\n", DDNS_LOGFILE);
            fclose(fp);
        }
#elif defined(CONFIG_PACKAGE_ez_ipupdate)
        if (!strcmp(srv, "noip")) {
            snprintf(server, SSBUF_LEN, "dynupdate.no-ip.com:80");
            snprintf(uri, SSBUF_LEN, "/nic/update");
        }
        else if (!strcmp(srv, "eurodyndns")) {
            snprintf(server, SSBUF_LEN, "eurodyndns.org:80");
            snprintf(uri, SSBUF_LEN, "/update/");
        }
        else if (!strcmp(srv, "dyndns")) {
            snprintf(server, SSBUF_LEN, "members.dyndns.org:80");
            snprintf(uri, SSBUF_LEN, "/nic/update");
        }
        else if (!strcmp(srv, "changeip")) {
            snprintf(server, SSBUF_LEN, "www.changeip.com");
            snprintf(uri, SSBUF_LEN, "http://nic.ChangeIP.com/nic/update");
        }
        else if (!strcmp(srv, "regfish")) {
            snprintf(server, SSBUF_LEN, "www.regfish.com:80");
            snprintf(uri, SSBUF_LEN, "/dyndns/2/");
        }
        else if (!strcmp(srv, "oray")) {
            snprintf(server, SSBUF_LEN, "phservice5.oray.net:6060");
            snprintf(uri, SSBUF_LEN, "/auth/dynamic.html");
        }
        else if (!strcmp(srv, "88ip")) {
            snprintf(server, SSBUF_LEN, "user.dipns.com:80");
            snprintf(uri, SSBUF_LEN, "/api/");
        }
        else if (!strcmp(srv, "changeip")) {
            snprintf(server, SSBUF_LEN, "www.changeip.com:80");
            snprintf(uri, SSBUF_LEN, "http://nic.ChangeIP.com/nic/update");
        }
        else if (!strcmp(srv, "domainhk")) {
            snprintf(server, SSBUF_LEN, "www.3domain.hk:80");
            snprintf(uri, SSBUF_LEN, "/areg.php");
        }
        else if (!strcmp(srv, "zoneedit")) {
            snprintf(server, SSBUF_LEN, "www.zoneedit.com:80");
            snprintf(uri, SSBUF_LEN, "/vanity/update");
        }
        else if (!strcmp(srv, "ovh")) {
            snprintf(server, SSBUF_LEN, "ovh.com:80");
            snprintf(uri, SSBUF_LEN, "/nic/update");
        }
        else if (!strcmp(srv, "qdns")) {
            snprintf(server, SSBUF_LEN, "members.3322.org:80");
            snprintf(uri, SSBUF_LEN, "/dyndns/update");
        }
        else if (!strcmp(srv, "dtdns")) {
            snprintf(server, SSBUF_LEN, "www.dtdns.com:80");
            snprintf(uri, SSBUF_LEN, "/api/autodns.cfm?");
        }
        else if (!strcmp(srv, "minidns")) {
            snprintf(server, SSBUF_LEN, "www.minidns.net:80");
            snprintf(uri, SSBUF_LEN, "/areg.php");
        }
        else if (!strcmp(srv, "tzo")) {
            snprintf(server, SSBUF_LEN, "cgi.tzo.com:80");
            snprintf(uri, SSBUF_LEN, "/webclient/signedon.html");
        }
        else if (!strcmp(srv, "hn")) {
            snprintf(server, SSBUF_LEN, "dup.hn.org:80");
            snprintf(uri, SSBUF_LEN, "/vanity/update");
        }
        else if (!strcmp(srv, "clearnet")) {
            snprintf(server, SSBUF_LEN, "members.clear-net.jp:80");
            snprintf(uri, SSBUF_LEN, "/ddns/nsupdate.php?");
        }
        else if (!strcmp(srv, "ez-ip")) {
            snprintf(server, SSBUF_LEN, "www.EZ-IP.Net:80");
            snprintf(uri, SSBUF_LEN, "/members/update/");
        }
        else if (!strcmp(srv, "ods")) {
            snprintf(server, SSBUF_LEN, "update.ods.org:7070");
            snprintf(uri, SSBUF_LEN, "update");
        }

        fp = fopen(DDNS_CONFILE, "w");
        if (fp) {
            fprintf(fp, "service-type=%s\n", DDNS_SERVICE);
            fprintf(fp, "server=%s\n", server);
            fprintf(fp, "user=%s:%s\n", un, pw);
            fprintf(fp, "host=%s\n", hn);
            fprintf(fp, "cache-file=%s\n", DDNS_CACHE);
            fprintf(fp, "pid-file=%s\n", DDNS_PIDFILE);
            fprintf(fp, "interface=%s\n", mtc->rule.WAN);
            fprintf(fp, "max-interval=%d\n", retry);
            fprintf(fp, "resolv-period=1\n");
            fprintf(fp, "period=1\n");
            fprintf(fp, "timeout=1.0\n");
            fprintf(fp, "request=%s\n", uri);
            fprintf(fp, "quiet\n");
            fclose(fp);
        }
#else
    (void) fp;
    (void) server;
    (void) uri;
    (void) un;
    (void) pw;
    (void) srv;
    (void) hn;
    (void) retry;
#endif
        exec_cmd2("start-stop-daemon -S -b -p %s -x %s -- %s", DDNS_PIDFILE, DDNS_CMDS, DDNS_ARGS);
    }
    else {
        ddns_stop();
        MTC_LOGGER(this, LOG_INFO, "ddns en=0");
    }

    return 0;

err_args:
    MTC_LOGGER(this, LOG_ERR, "load args fail, ddns1=[%s]", ddns1);
    return 0;
}

static int start(void)
{
#if defined(CONFIG_PACKAGE_inadyn) || defined(CONFIG_PACKAGE_ez_ipupdate)
    return ddns_start();
#else
    return 0;
#endif
}

static int stop(void)
{
#if defined(CONFIG_PACKAGE_inadyn) || defined(CONFIG_PACKAGE_ez_ipupdate)
    return ddns_stop();
#else
    return 0;
#endif
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int ddns_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

