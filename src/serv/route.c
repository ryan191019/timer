#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h>

#include <arpa/inet.h>
#include <netutils.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

extern MtcData *mtc;

#define NETROUTE_FILE    "/proc/net/route"
#ifndef RTF_UP
/* Keep this in sync with /usr/src/linux/include/linux/route.h */
#define RTF_UP          0x0001  /* route usable                 */
#define RTF_GATEWAY     0x0002  /* destination is a gateway     */
#define RTF_HOST        0x0004  /* host entry (net otherwise)   */
#endif

static int FUNCTION_IS_NOT_USED route_get_network(char *devname, unsigned long *ip, unsigned long *mask, unsigned long *network)
{
    char info[30];
    struct in_addr ip_addr, mask_addr;

    if (get_ifname_ether_ip(devname, info, sizeof(info)) != EXIT_SUCCESS) {
        return -1;
    }
    else {
        inet_aton(info, &ip_addr);
    }
    if (get_ifname_ether_mask(devname, info, sizeof(info)) != EXIT_SUCCESS) {
        return -1;
    }
    else {
        inet_aton(info, &mask_addr);
    }

    if (ip) {
        *ip = ip_addr.s_addr;
    }
    if (mask) {
        *mask = mask_addr.s_addr;
    }
    if (network) {
        *network = ip_addr.s_addr & mask_addr.s_addr;
    }

    return 0;
}

static int FUNCTION_IS_NOT_USED route_reinit(void)
{
    char dst_addrbuf[INET6_ADDRSTRLEN+1];
    char gate_addrbuf[INET6_ADDRSTRLEN+1];
    char devname[64];
    unsigned long d, g, m;
    int flgs, ref, use, metric, mtu, win, ir;
    struct sockaddr_in s_addr;
    struct in_addr mask;
#if 0
    unsigned long dev_ip, dev_mask, dev_network;
#endif
    FILE *fp;

    if( !f_exists(NETROUTE_FILE) ) {
        return 0;
    }

    printf("Destination     Gateway         Genmask         Iface\n");

    fp = fopen(NETROUTE_FILE, "r");

    if (fscanf(fp, "%*[^\n]\n") < 0) { /* Skip the first line. */
        goto ERROR;        /* Empty or missing line, or read error. */
    }
    while (1) {
        int r;
        r = fscanf(fp, "%63s%lx%lx%X%d%d%d%lx%d%d%d\n",
                   devname, &d, &g, &flgs, &ref, &use, &metric, &m,
                   &mtu, &win, &ir);
        if (r != 11) {
            if ((r < 0) && feof(fp)) { /* EOF with no (nonspace) chars read. */
                break;
            }
        }
        else {
            memset(&s_addr, 0, sizeof(struct sockaddr_in));
            s_addr.sin_family = AF_INET;
            s_addr.sin_addr.s_addr = d;
            if (!inet_ntop( AF_INET, (void *)&(s_addr.sin_addr), dst_addrbuf, sizeof(dst_addrbuf) )) {
                continue;
            }
            else {
                printf("%-15.15s ", dst_addrbuf);
            }

            s_addr.sin_addr.s_addr = g;
            if (!inet_ntop( AF_INET, (void *)&(s_addr.sin_addr), gate_addrbuf, sizeof(gate_addrbuf) )) {
                continue;
            }
            else {
                printf("%-15.15s ", gate_addrbuf);
            }

            mask.s_addr = m;
            printf("%-16s%-6s\n", inet_ntoa(mask), devname);

#if 1
            if ((flgs != RTF_UP) && (d != 0)) {
                printf("route del: d[%s]:m[%d]:i[%s]:g[%s]:n[%s]\n", 
                        devname, metric, dst_addrbuf, gate_addrbuf, inet_ntoa(mask));
                route_del(devname, 0, dst_addrbuf, gate_addrbuf, inet_ntoa(mask));
            }
#else
            if (!route_get_network(devname, &dev_ip, &dev_mask, &dev_network)) {
#if defined(ROUTE_DGB)
                printf("route: [%x]:[%x]\n", (unsigned int)m, (unsigned int)(d & m));
                printf("dev  : [%x]:[%x] ip:[0x%x]\n", 
                        (unsigned int)dev_mask, (unsigned int)dev_network, (unsigned int)dev_ip);
#endif
                if ((dev_mask != m) || (dev_network != (d & m))) {
                    printf("route del: d[%s]:m[%d]:i[%s]:g[%s]:n[%s]\n", 
                            devname, metric, dst_addrbuf, gate_addrbuf, inet_ntoa(mask));
                    route_del(devname, metric, dst_addrbuf, gate_addrbuf, inet_ntoa(mask));
                }
            }
#endif
        }
    }

ERROR:
    fclose(fp);

    return 0;
}

static int FUNCTION_IS_NOT_USED route_build_static_route(void)
{
// $route_entry1=ip=50.50.50.50&nm=100.100.100.100&gw=200.200.200.200&en=1&metric=2
    char route_entry[MSBUF_LEN] = { 0 };
    char *argv[10];
    char *ip, *nm, *gw, *en, *metric;
    int num;
    int rule;
    int i;

    for (i=1, rule=0;;i++, rule++) {
        if (cdb_get_array_str("$route_entry", i, route_entry, sizeof(route_entry), NULL) == NULL) {
            break;
        }
        else if (strlen(route_entry) == 0) {
            break;
        }
        else {
            MTC_LOGGER(this, LOG_INFO, "%s: route_entry%d is [%s]", __func__, i, route_entry);

            num = str2argv(route_entry, argv, '&');
            if((num < 5)                 || !str_arg(argv, "ip=")   || 
               !str_arg(argv, "nm=")     || !str_arg(argv, "gw=")   || 
               !str_arg(argv, "en=")     || !str_arg(argv, "metric=")) {
                MTC_LOGGER(this, LOG_INFO, "%s: skip, argv is not complete", __func__);
                continue;
            }

            ip     = str_arg(argv, "ip=");
            nm     = str_arg(argv, "nm=");
            gw     = str_arg(argv, "gw=");
            en     = str_arg(argv, "en=");
            metric = str_arg(argv, "metric=");

            MTC_LOGGER(this, LOG_INFO, "%s: en is [%s]", __func__, en);
            MTC_LOGGER(this, LOG_INFO, "%s: ip is [%s]", __func__, ip);
            MTC_LOGGER(this, LOG_INFO, "%s: nm is [%s]", __func__, nm);
            MTC_LOGGER(this, LOG_INFO, "%s: gw is [%s]", __func__, gw);
            MTC_LOGGER(this, LOG_INFO, "%s: metric is [%s]", __func__, metric);

            printf("route add: d[%s]:m[%s]:i[%s]:g[%s]:n[%s]\n", 
                    "", metric, ip, gw, nm);
            route_add(NULL, atoi(metric), ip, gw, nm);
        }
    }

    return 0;
}

#if defined(CONFIG_PACKAGE_quagga)
#define QUAGGA_NAME     "quagga"
#define QUAGGA_PROG     "/etc/init.d/"QUAGGA_NAME
#define QUAGGA_INIT     "/usr/sbin/"QUAGGA_NAME".init"
#define QUAGGA_LOGDIR   "/var/log/"QUAGGA_NAME
#define QUAGGA_LOGFILE  QUAGGA_LOGDIR"/"QUAGGA_NAME".log"
#define ZEBRA_CONFILE   "/etc/"QUAGGA_NAME"/zebra.conf"
#define RIPD_CONFILE    "/etc/"QUAGGA_NAME"/ripd.conf"

static int FUNCTION_IS_NOT_USED quagga_save_conf(void)
{
#define MAX_RIP_VERSION_NUM 3
    char *rip_version[] = {
        "1",
        "1 2",
        "2",
        "no router rip",
    };
    FILE *fp;
    int route_rip_rx = cdb_get_int("$route_rip_rx", 0);
    int route_rip_tx = cdb_get_int("$route_rip_tx", 0);

    if ((route_rip_rx < 1) || (route_rip_rx > MAX_RIP_VERSION_NUM)) {
        route_rip_rx = MAX_RIP_VERSION_NUM;
    }
    if ((route_rip_tx < 1) || (route_rip_tx > MAX_RIP_VERSION_NUM)) {
        route_rip_tx = MAX_RIP_VERSION_NUM;
    }

    fp = fopen(QUAGGA_LOGFILE, "w");
    if (fp) {
        fclose(fp);
    }
    fp = fopen(ZEBRA_CONFILE, "w");
    if (fp) {
        fclose(fp);
    }

    fp = fopen(RIPD_CONFILE, "w");
    if (fp) {
        fprintf(fp, "router rip\n");
        if (strlen(mtc->rule.LAN) > 0) {
            fprintf(fp, "network %s\n", mtc->rule.LAN);
            fprintf(fp, "interface %s\n", mtc->rule.LAN);
            fprintf(fp, "no ip rip authentication mode\n");
        }
        if (strlen(mtc->rule.WAN) > 0) {
            fprintf(fp, "network %s\n", mtc->rule.WAN);
            fprintf(fp, "interface %s\n", mtc->rule.WAN);
            fprintf(fp, "no ip rip authentication mode\n");
        }
        if (strlen(mtc->rule.WANB) > 0) {
            fprintf(fp, "network %s\n", mtc->rule.WANB);
            fprintf(fp, "interface %s\n", mtc->rule.WANB);
            fprintf(fp, "no ip rip authentication mode\n");
        }
        fprintf(fp, "ip rip receive version %s\n", rip_version[route_rip_rx]);
        fprintf(fp, "ip rip send version %s\n", rip_version[route_rip_tx]);

        fclose(fp);
    }

    return 0;
}

/*
 * #*************************************************
 * # quagga doesn't support:
 * # 1.route_rip_mode
 * # 2.only disable one of rx or tx
 * #*************************************************
 */
static int FUNCTION_IS_NOT_USED quagga_start(void)
{
    int route_rip_en = cdb_get_int("$route_rip_en", 0);

    if (!route_rip_en || !f_exists(QUAGGA_PROG) || !f_exists(QUAGGA_INIT)) {
        return 0;
    }

    exec_cmd2("sed -i '/DAEMON_FLAGS=/s/=.*$/=\"-d -u root\"/' "QUAGGA_INIT);

    if (!f_isdir(QUAGGA_LOGDIR)) {
        mkdir(QUAGGA_LOGDIR, 0755);
    }
    quagga_save_conf();
    exec_cmd2(QUAGGA_PROG" start");

    return 0;
}

static int FUNCTION_IS_NOT_USED quagga_stop(void)
{
    exec_cmd2(QUAGGA_PROG" stop");

    return 0;
}
#endif

static int start(void)
{
    route_build_static_route();
#if defined(CONFIG_PACKAGE_quagga)
    quagga_start();
#endif
    return 0;
}

static int stop(void)
{
#if defined(CONFIG_PACKAGE_quagga)
    quagga_stop();
#endif
    route_reinit();
    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int route_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

