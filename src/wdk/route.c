#include <ctype.h>
#include <arpa/inet.h>
#include <netutils.h>
#include "wdk.h"

#define NETROUTE_FILE    "/proc/net/route"
#ifndef RTF_UP
/* Keep this in sync with /usr/src/linux/include/linux/route.h */
#define RTF_UP          0x0001  /* route usable                 */
#define RTF_GATEWAY     0x0002  /* destination is a gateway     */
#define RTF_HOST        0x0004  /* host entry (net otherwise)   */
#endif


static int __attribute__ ((unused)) route_show_raw(void)
{
// dst=192.168.169.0&nm=255.255.255.0&gw=0.0.0.0&flg=U&if=br0None
    char dst_addrbuf[INET6_ADDRSTRLEN+1];
    char gate_addrbuf[INET6_ADDRSTRLEN+1];
    char devname[64];
    unsigned long d, g, m;
    int flgs, ref, use, metric, mtu, win, ir;
    struct sockaddr_in s_addr;
    struct in_addr mask;
    FILE *fp;

    if( !f_exists(NETROUTE_FILE) ) {
        return 0;
    }

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

            s_addr.sin_addr.s_addr = g;
            if (!inet_ntop( AF_INET, (void *)&(s_addr.sin_addr), gate_addrbuf, sizeof(gate_addrbuf) )) {
                continue;
            }

            mask.s_addr = m;

            if (!(flgs & RTF_UP)) { /* Skip interfaces that are down. */
                continue;
            }

            printf("dst=%s&nm=%s&gw=%s&flg=U&if=%sNone\n", dst_addrbuf, inet_ntoa(mask), gate_addrbuf, devname);
        }
    }

ERROR:
    fclose(fp);

    return 0;
}

int wdk_route(int argc, char **argv)
{
    int err = -1;

    if(argc == 1) {
        if (strcmp("raw", argv[0]) == 0) {
            route_show_raw();
            err = 0;
        } else if (strcmp("rip", argv[0]) == 0)  {
            exec_cmd2("mtc_cli route restart");
            err = 0;
        }
    }

    if (!err) {
        return 0;
    }

    printf("usage:/lib/wdk/route [raw/rip]\n");

    return -1;
}

