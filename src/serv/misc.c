#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"
#include "misc.h"

extern MtcData *mtc;

int system_update_hosts(void)
{
    char sys_name[SSBUF_LEN];
    char host_name[SSBUF_LEN];
    char ip[30];
    if (cdb_get_str("$sys_name", sys_name, sizeof(sys_name), NULL) == NULL) {
        if (exec_cmd3(host_name, sizeof(host_name), 
                "cat /proc/sys/kernel/hostname") == 0) {
            if (strcmp(host_name, sys_name)) {
                f_wsprintf("/proc/sys/kernel/hostname", "%s\n",sys_name );
                if (strcmp(host_name, "(none)")) {
                    // it is not in boot process now
                    // hostname is changed, need to update /etc/hosts
                    f_write_string("/etc/hosts", "127.0.0.1 localhost\n" , 0, 0);
                    get_ifname_ether_ip(mtc->rule.LAN, ip, sizeof(ip));
                    f_wsprintf("/etc/hosts", "%s %s\n",ip ,sys_name );
  
                }
            }
        }
    }

    return 0;
}


