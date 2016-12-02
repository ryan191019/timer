#include <ctype.h>
#include "wdk.h"

#include "include/mtc_linklist.h"

#define DNS_NAME        "dnsmasq"
#define DNS_CONFILE     "/tmp/"DNS_NAME".conf"
#define DNS_LEASEFILE   "/var/run/"DNS_NAME".leases"

#define MYTAG           "MODIFIED"

struct leaseItem {
    char *addr;
    const char *mac;
    const char *name;
    const char *ip;
    int flg;
    int exp;
};

typedef struct {
    struct list_head list;
    struct leaseItem item;
}leaseListNode;

static int __attribute__ ((unused)) dhcps_get_leases_num(void)
{
    int num = 0;
    FILE *fp;
    char c;

    if( f_exists(DNS_LEASEFILE) ) {
        fp = fopen(DNS_LEASEFILE, "r");
        if (fp) {
            while ((c = fgetc(fp)) != EOF) {
                if (c == '\n') {
                    num++;
                }
            }
            fclose(fp);
        }
    }

    return num;
}

static int __attribute__ ((unused)) dhcps_parse_dnsmasq_conf(struct list_head *lh)
{
//#output     dhcp-host=00:50:80:60:50:30,nady-testpc,192.168.0.50
    leaseListNode *node = NULL;
    FILE *fp = NULL;
    char buf[128];
    char *argv[10];
    char *addr = NULL;
    int tag = 0;

    if( !f_exists(DNS_CONFILE) ) {
        return 0;
    }
    if ((fp = fopen(DNS_CONFILE, "r")) != NULL) {
        while (new_fgets(buf, sizeof(buf), fp) != NULL) {
            if (!tag && strstr(buf, MYTAG)) {
                tag = 1;
            }
            if (strstr(buf, "dhcp-host=")) {
                if ((addr = strdup(buf)) == NULL)
                    continue;
                if (str2argv(addr+sizeof("dhcp-host=")-1, argv, ',') < 3)
                    continue;

                if ((node = (leaseListNode *)calloc(sizeof(leaseListNode), 1)) != NULL) {
                    node->list.next = node->list.prev = &node->list;
                    node->item.addr = addr;
                    node->item.mac = argv[0];
                    node->item.name = argv[1];
                    node->item.ip = argv[2];
                    node->item.flg = 1;
                    node->item.exp = 0;

                    list_add_tail(&node->list, lh);
                }
            }
        }
        fclose(fp);
    }
    if (tag) {
        exec_cmd2("sed -i '/%s/d' %s", MYTAG, DNS_CONFILE);
        exec_cmd2("mtc_cli dns restart");
    }

    return 0;
}

static int __attribute__ ((unused)) dhcps_parse_dnsmasq_leases(struct list_head *lh)
{
//#leasefile  97789 00:05:5d:82:b5:b7 192.168.0.108 nady-desktop *
    leaseListNode *node;
    FILE *fp;
    char buf[128];
    char *argv[10];
    char *addr = NULL;

    if( !f_exists(DNS_LEASEFILE) ) {
        return 0;
    }
    if ((fp = fopen(DNS_LEASEFILE, "r")) != NULL) {
        while (new_fgets(buf, sizeof(buf), fp) != NULL) {
            if ((addr = strdup(buf)) == NULL)
                continue;
            if (str2argv(addr, argv, ' ') < 4)
                continue;

            if ((node = (leaseListNode *)calloc(sizeof(leaseListNode), 1)) != NULL) {
                node->list.next = node->list.prev = &node->list;
                node->item.addr = addr;
                node->item.mac = argv[1];
                node->item.name = argv[3];
                node->item.ip = argv[2];
                node->item.flg = 0;
                node->item.exp = atoi(argv[0]);

                list_add_tail(&node->list, lh);
            }
        }
        fclose(fp);
    }

    return 0;
}

static int __attribute__ ((unused)) dhcps_show_leases(void)
{
//#leasefile  97789 00:05:5d:82:b5:b7 192.168.0.108 nady-desktop *
//#output     dhcp-host=00:50:80:60:50:30,nady-testpc,192.168.0.50
//#show       name=nady-desktop&ip=192.168.0.108&mac=00:05:5d:82:b5:b7&flg=0&exp=97789
    LIST_HEAD(client);
    struct list_head *lh = &client;
    struct list_head *pos, *next;
    leaseListNode *node, *nnode;
    struct leaseItem item;
    int repeat = 0;

    dhcps_parse_dnsmasq_conf(lh);
    dhcps_parse_dnsmasq_leases(lh);

    if (!list_empty(lh)) {
        do {
            repeat = 0;
            list_for_each_safe(pos, next, lh) {
                if (next != lh) {
                    node = (leaseListNode *)list_entry(pos, leaseListNode, list);
                    nnode = (leaseListNode *)list_entry(next, leaseListNode, list);
                    if (strcmp(node->item.mac, nnode->item.mac) > 0) {
                        memcpy(&item, &node->item, sizeof(struct leaseItem));
                        memcpy(&node->item, &nnode->item, sizeof(struct leaseItem));
                        memcpy(&nnode->item, &item, sizeof(struct leaseItem));

                        repeat = 1;
                    }
                }
            }
        } while (repeat);

        list_for_each_safe(pos, next, lh) {
            node = (leaseListNode *)list_entry(pos, leaseListNode, list);
            printf("name=%s&ip=%s&mac=%s&flg=%d&exp=%d\n", 
                    node->item.name, node->item.ip, node->item.mac, node->item.flg, node->item.exp);
            list_del(&node->list);
            free(node->item.addr);
            free(node);
        }
    }

    return 0;
}

static int __attribute__ ((unused)) dhcps_set(char *args)
{
// name=Client2&ip=192.168.169.200&mac=00:11:22:33:44:50&flg=1
    char *argv[10];
    char *name, *ip, *mac, *flg;
    char *p;

    if( !f_exists(DNS_CONFILE) ) {
        return 1;
    }

    if((str2argv(args, argv, '&') < 4) || 
       !str_arg(argv, "name=") || !str_arg(argv, "ip=") || 
       !str_arg(argv, "mac=")  || !str_arg(argv, "flg=")) {
        printf("%s: skip, argv is not complete", __func__);
        return 1;
    }

    name = str_arg(argv, "name=");
    ip   = str_arg(argv, "ip=");
    mac  = str_arg(argv, "mac=");
    flg  = str_arg(argv, "flg=");
    for (p=mac; *p; ++p) *p = tolower(*p);

    exec_cmd2("sed -i '/dhcp-host=%s,.*,%s/d' %s", mac, ip, DNS_CONFILE);
    if (!strcmp(flg, "1")) {
        exec_cmd2("sed -i '$a dhcp-host=%s,%s,%s' %s", mac, name, ip, DNS_CONFILE);
    }
    exec_cmd2("echo %s >> %s", MYTAG, DNS_CONFILE);

    return 0;
}

int wdk_dhcps(int argc, char **argv)
{
    int err = -1;

    if(argc == 1) {
        if (strcmp("leases", argv[0]) == 0) {
            dhcps_show_leases();
            err = 0;
        } else if (strcmp("num", argv[0]) == 0)  {
            printf("%d\n", dhcps_get_leases_num());
            err = 0;
        }
    } else if(argc == 2) {
        if (strcmp("set", argv[0]) == 0) {
            dhcps_set(argv[1]);
            err = 0;
        }
    }

    if (!err) {
        return 0;
    }

    printf("usage:/lib/wdk/dhcps [set/leases/num]\n");
    printf("/lib/wdk/dhcps set name=Client1&ip=192.168.169.100&mac=00:11:22:33:44:55&flg=1\n");

    return -1;
}

