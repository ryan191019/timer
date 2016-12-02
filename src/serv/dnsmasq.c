#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"
#include "misc.h"

extern MtcData *mtc;

#define DNS_NAME        "dnsmasq"
#define DNS_CMDS        "/usr/sbin/"DNS_NAME
#define DNS_ARGS        "-C /tmp/"DNS_NAME".conf"
#define DNS_CONFILE     "/tmp/"DNS_NAME".conf"
#define DNS_PIDFILE     "/var/run/"DNS_NAME".pid"
#define DNS_LOGFILE     "/var/run/"DNS_NAME".log"
#define DNS_LEASEFILE   "/var/run/"DNS_NAME".leases"

#define DNS_NEW_CONFILE "/tmp/"DNS_NAME".conf.new"

#define RESOLV_FILE     "/etc/resolv.conf"

static char * FUNCTION_IS_NOT_USED dnsmasq_transform_from_sec(int sec)
{
    static char dnsmasq_lease_time[USBUF_LEN];

    switch(sec) {
        case 1800:
            sprintf(dnsmasq_lease_time, "30m");
            break;
        case 3600:
            sprintf(dnsmasq_lease_time, "1h");
            break;
        case 7200:
            sprintf(dnsmasq_lease_time, "2h");
            break;
        case 28800:
            sprintf(dnsmasq_lease_time, "8h");
            break;
        case 43200:
            sprintf(dnsmasq_lease_time, "12h");
            break;
        case 86400:
            sprintf(dnsmasq_lease_time, "1d");
            break;
        case 172800:
            sprintf(dnsmasq_lease_time, "2d");
            break;
        case 604800:
            sprintf(dnsmasq_lease_time, "7d");
            break;
        case 1209600:
            sprintf(dnsmasq_lease_time, "14d");
            break;
        case 315360000:
        default:
            sprintf(dnsmasq_lease_time, "infinite");
            break;
    }
    return dnsmasq_lease_time;
}

int dnsmasq_save_conf(int dns_enable, int dhcps_enable)
{
    FILE *fp, *fpb;
    char dhcps_start[PBUF_LEN];
    char dhcps_end[PBUF_LEN];
    char dns_def[PBUF_LEN];
    char dns_svr1[PBUF_LEN];
    char dns_svr2[PBUF_LEN];
    char lan_ip[PBUF_LEN];
    char wanif_domain[USBUF_LEN];
    char buf[MSBUF_LEN];
    char *ptr;
    int dhcps_lease_time = cdb_get_int("$dhcps_lease_time", 0);
    int dhcps_probe = cdb_get_int("$dhcps_probe", 0);
    int dhcps_hide_gw = cdb_get_int("$dhcps_hide_gw", 0);
    int dns_fix = cdb_get_int("$dns_fix", 0);

    if ((mtc->rule.OMODE == opMode_rp) || (mtc->rule.OMODE == opMode_br)) {
        dhcps_enable = 0;
    }

    fp = fopen(DNS_NEW_CONFILE, "w");
    if (fp) {
        if ((cdb_get_str("$dhcps_start", dhcps_start, sizeof(dhcps_start), NULL) != NULL) && 
            (cdb_get_str("$dhcps_end", dhcps_end, sizeof(dhcps_end), NULL) != NULL) && 
            (dhcps_enable == 1)) {
            fprintf(fp, "dhcp-range=%s,%s,%s\n", dhcps_start, dhcps_end, 
                    dnsmasq_transform_from_sec(dhcps_lease_time));
        }
        fprintf(fp, "dhcp-leasefile=%s\n", DNS_LEASEFILE);
        if (dhcps_hide_gw) {
            fprintf(fp, "dhcp-option=6\n");
            fprintf(fp, "dhcp-option=3\n");
        }
        else {
            int dns_default = 1;
            fprintf(fp, "dhcp-option=6,");
            if ((dns_enable) &&
                (cdb_get_str("$lan_ip", lan_ip, sizeof(lan_ip), NULL) != NULL)) {
                fprintf(fp, "%s", lan_ip);
                dns_default = 0;
            }
            else {
                if (dns_fix) {
                    if ((cdb_get_str("$dns_svr1", dns_svr1, sizeof(dns_svr1), NULL) != NULL) &&
                        (strcmp(dns_svr1, ""))){
                        fprintf(fp, "%s", dns_svr1);
                        if ((cdb_get_str("$dns_svr2", dns_svr2, sizeof(dns_svr2), NULL) != NULL) &&
                            (strcmp(dns_svr2, ""))){
                            fprintf(fp, ",%s", dns_svr2);
                        }
                        dns_default = 0;
                    }
                }
                else {
                    fpb = fopen(RESOLV_FILE, "r");
                    if (fpb) {
                        while ((ptr = new_fgets(buf, sizeof(buf), fpb)) != NULL) {
                            if(!strncmp(ptr, "nameserver", sizeof("nameserver")-1)) {
                                ptr += sizeof("nameserver");
                                if (!dns_default) {
                                    fprintf(fp, ",");
                                }
                                fprintf(fp, "%s", ptr);
                                dns_default = 0;
                            }
                        }
                        fclose(fpb);
                    }
                }
            }
            if ((dns_default) && 
                (cdb_get_str("$dns_def", dns_def, sizeof(dns_def), NULL) != NULL)) {
                fprintf(fp, "%s", dns_def);
            }
            fprintf(fp, "\n");
        }

        if (cdb_get_str("$lan_ip", lan_ip, sizeof(lan_ip), NULL) != NULL) {
            fprintf(fp, "listen-address=%s\n", lan_ip);
        }

        if ((cdb_get_str("$wanif_domain", wanif_domain, sizeof(wanif_domain), NULL) != NULL) && 
            (strcmp(wanif_domain, ""))) {
            fprintf(fp, "domain=%s\n", wanif_domain);
        }

        fprintf(fp, "bind-interfaces\n");

        if (dhcps_probe) {
            MTC_LOGGER(this, LOG_INFO, "********************************************");
            MTC_LOGGER(this, LOG_INFO, "* DHCP Server probe is not implemented yet *");
            MTC_LOGGER(this, LOG_INFO, "********************************************");
        }

        fpb = fopen(DNS_CONFILE, "r");
        if (fpb) {
            while ((ptr = new_fgets(buf, sizeof(buf), fpb)) != NULL) {
                if(!strncmp(ptr, "dhcp-host=", sizeof("dhcp-host=")-1)) {
                    fprintf(fp, "%s\n", ptr);
                }
            }
            fclose(fpb);
        }

        fclose(fp);
    }
    else {
        MTC_LOGGER(this, LOG_ERR, "Can't open %s", DNS_NEW_CONFILE);
        return 1;
    }

    exec_cmd2("mv %s %s", DNS_NEW_CONFILE, DNS_CONFILE);

    return 0;
}

static int start(void)
{
    char args[SSBUF_LEN] = DNS_ARGS" --port=0";
    int dns_enable = cdb_get_int("$dns_enable", 0);
    int dhcps_enable = cdb_get_int("$dhcps_enable", 0);

    if (dns_enable == 1) {
        snprintf(args, sizeof(args), "%s", DNS_ARGS);
    }

    system_update_hosts();
    dnsmasq_save_conf(dns_enable, dhcps_enable);

    exec_cmd2("start-stop-daemon -S -p %s -x %s -- %s", DNS_PIDFILE, DNS_CMDS, args);

    return 0;
}

static int stop(void)
{
    if( f_exists(DNS_PIDFILE) ) {
        exec_cmd2("start-stop-daemon -K -q -s 9 -p %s", DNS_PIDFILE);
        unlink(DNS_PIDFILE);
    }
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int dnsmasq_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

