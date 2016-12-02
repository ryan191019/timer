#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

extern MtcData *mtc;

#define UHTTPD_NAME     "uhttpd"
#define UHTTPD_CMDS     "/usr/sbin/"UHTTPD_NAME
#define UHTTPD_RAM_CMDS "/tmp/"UHTTPD_NAME
#define UHTTPD_KEY      "/var/uhttpd.key"
#define UHTTPD_CERT     "/var/uhttpd.crt"
#define UHTTPD_CONFILE  "/tmp/"UHTTPD_NAME".conf"
#define UHTTPD_PIDFILE  "/var/run/"UHTTPD_NAME".pid"
#define UHTTPD_ARGS     "-f -c "UHTTPD_CONFILE" -h /www -p 0.0.0.0:80"
#define UHTTPSD_ARGS    UHTTPD_ARGS" -K "UHTTPD_KEY" -C "UHTTPD_CERT" -s 0.0.0.0:443"

#define UHTTPD_TLSFILE  "/usr/lib/uhttpd_tls.so"
#define PX5G_BIN        "/usr/sbin/px5g"

static int FUNCTION_IS_NOT_USED uhttpd_generate_keys(void)
{
/*
	local days=730
	local bits=1024
	local country="DE"
	local state="Berlin"
	local location="Berlin"
	local commonname="OpenWrt"

	[ -x "$PX5G_BIN" ] && {
		$PX5G_BIN selfsigned -der \
			-days ${days:-730} -newkey rsa:${bits:-1024} -keyout "$UHTTPD_KEY" -out "$UHTTPD_CERT" \
			-subj /C=${country:-DE}/ST=${state:-Saxony}/L=${location:-Leipzig}/CN=${commonname:-OpenWrt}
	} || {
		echo "WARNING: the specified certificate and key" \
			"files do not exist and the px5g generator" \
			"is not available, skipping SSL setup."
	}
 */

    if( f_exists(PX5G_BIN) ) {
        exec_cmd2(PX5G_BIN" selfsigned -der -days 730 -newkey rsa:1024 -keyout "UHTTPD_KEY" -out "UHTTPD_CERT
                  " -subj /C=DE/ST=Berlin/L=Berlin/CN=OpenWrt");
    }
    else {
        MTC_LOGGER(this, LOG_ERR, "WARNING: the specified certificate and key");
        MTC_LOGGER(this, LOG_ERR, "files do not exist and the px5g generator");
        MTC_LOGGER(this, LOG_ERR, "is not available, skipping SSL setup.");
    }

    return 0;
}

static int start(void)
{
    FILE *fp;
    char args[MSBUF_LEN] = UHTTPD_ARGS;
    char sysuser[MSBUF_LEN];
    char *argv[10];
    char *user, *pass;

    if( !f_exists(UHTTPD_RAM_CMDS) ) {
        cp(UHTTPD_CMDS, UHTTPD_RAM_CMDS);
    }

    if( f_exists(UHTTPD_TLSFILE) ) {
        if( !f_exists(UHTTPD_KEY) && !f_exists(UHTTPD_CERT) ) {
            uhttpd_generate_keys();
        }
        if( f_exists(UHTTPD_KEY) && f_exists(UHTTPD_CERT) ) {
            snprintf(args, sizeof(args), "%s", UHTTPSD_ARGS);
        }
    }

    cdb_get_str("$sys_user1", sysuser, sizeof(sysuser), NULL);
    str2argv(sysuser, argv, '&');
    user = str_arg(argv, "user=");
    pass = str_arg(argv, "pass=");
    if (!user || !pass) {
        MTC_LOGGER(this, LOG_ERR, "invalid $sys_user1=[%s]", sysuser);
        return 0;
    }
    fp = fopen(UHTTPD_CONFILE, "w");
    if (fp) {
        fprintf(fp, "/:%s:%s\n", user, pass);
        fclose(fp);
    }

    exec_cmd2("start-stop-daemon -S -p %s -x %s -m -b -- %s", UHTTPD_PIDFILE, UHTTPD_RAM_CMDS, args);

    return 0;
}

static int stop(void)
{
    if( f_exists(UHTTPD_PIDFILE) ) {
        exec_cmd2("start-stop-daemon -K -q -s TERM -n %s -p %s", UHTTPD_NAME, UHTTPD_PIDFILE);
        unlink(UHTTPD_PIDFILE);
    }
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int uhttpd_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

