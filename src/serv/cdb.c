/*
 * =====================================================================================
 *
 *       Filename:  checkup.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  01/22/2016 05:33:21 PM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

/*=====================================================================================+
 | Included Files                                                                      |
 +=====================================================================================*/

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <pthread.h>
#include <time.h>
#include <sys/reboot.h>
#include <string.h>

#include "include/mtc.h"
#include "include/mtc_proc.h"

/*=====================================================================================+
 | Define                                                                              |
 +=====================================================================================*/

#define FCDB_FILE "/tmp/run/config.cdb"

typedef struct {
    const char *cname;
    void (*exec_proc)(char *kcdb, char *fcdb, int clear);
} montage_checkup_t;

/*=====================================================================================+
 | Variables                                                                           |
 +=====================================================================================*/

static int iscdbsave = 0;
static int isneedreboot = 0;

/*=====================================================================================+
 | Function Prototypes                                                                 |
 +=====================================================================================*/

void lan_ip_proc(char *kcdb, char *fcdb, int clear);
void remote_ctrl_proc(char *kcdb, char *fcdb, int clear);
void wan_clone_mac_proc(char *kcdb, char *fcdb, int clear);
void op_work_proc(char *kcdb, char *fcdb, int clear);

montage_checkup_t checkup_tbl[] = {
    { "$lan_ip",               lan_ip_proc        },
    { "$sys_remote_enable",    remote_ctrl_proc   },
    { "$sys_remote_ip",        remote_ctrl_proc   },
    { "$sys_remote_port",      remote_ctrl_proc   },
    { "$wan_clone_mac",        wan_clone_mac_proc },
    { "$wan_clone_mac_enable", wan_clone_mac_proc },
    { "$op_work_mode",         op_work_proc       },
};

/*=====================================================================================+
 | Extern Function/Variables                                                           |
 +=====================================================================================*/

/*=====================================================================================+
 | Functions                                                                           |
 +=====================================================================================*/

void lan_ip_proc(char *kcdb, char *fcdb, int clear)
{
    static int done = 0;
    if (clear) {
        done = 0;
    }
    else if (!done) {
        char lanip[64];
        char dhcps_start[64];
        char dhcps_end[64];
        char dhcps_ip[64];
        char *p1 = lanip, *p2 = dhcps_start, *p3 = dhcps_end;
        cdb_get("$lan_ip", lanip);
        cdb_get("$dhcps_start", dhcps_start);
        cdb_get("$dhcps_end", dhcps_end);
        if ((p1 = strrchr(lanip, '.'))) {
            *p1 = '\0';
        }
        if ((p2 = strrchr(dhcps_start, '.'))) {
            *p2 = '\0';
        }
        if ((p3 = strrchr(dhcps_end, '.'))) {
            *p3 = '\0';
        }
        if (!p1 || !p2 || !p3) {
            return; 
        }
        if (strcmp(lanip, dhcps_start)) {
            sprintf(dhcps_ip, "%s.%s", lanip, p2+1);
            cdb_set("$dhcps_start", dhcps_ip);
            sprintf(dhcps_ip, "%s.%s", lanip, p3+1);
            cdb_set("$dhcps_end", dhcps_ip);
            isneedreboot = 1;
        }
        done = 1;
    }
}

void remote_ctrl_proc(char *kcdb, char *fcdb, int clear)
{
    static int done = 0;
    if (clear) {
        done = 0;
    }
    else if (!done) {
        char tmp[BUF_SIZE];
        cdb_get("$nat_enable", tmp);
        cdb_set("$nat_enable", tmp);
        done = 1;
    }
}

void wan_clone_mac_proc(char *kcdb, char *fcdb, int clear)
{
    static int done = 0;
    if (clear) {
        done = 0;
    }
    else if (!done) {
        isneedreboot = 1;
        done = 1;
    }
}

void op_work_proc(char *kcdb, char *fcdb, int clear)
{
    static int done = 0;
    if (clear) {
        done = 0;
    }
    else if (!done) {
        int kop_work = atoi(kcdb);
        int fop_work = atoi(fcdb);
        switch(kop_work) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 6:
            case 7:
            case 9:
                {
                    switch(fop_work) {
                        case 4:
                        case 5:
                        case 8:
                            cdb_set("$nat_enable", "1");
                            cdb_set("$fw_enable", "1");
                            cdb_set("$dhcps_enable", "1");
                            cdb_set("$wl_forward_mode", "0");
                            break;
                        default:
                            break;
                    }
                }
                break;
            case 4:
            case 5:
            case 8:
                {
                    switch(fop_work) {
                        case 3:
                        case 4:
                        case 5:
                        case 9:
                            cdb_set("$nat_enable", "0");
                            cdb_set("$fw_enable", "0");
                            cdb_set("$dhcps_enable", "0");
                            cdb_set("$wl_forward_mode", "1");
                            break;
                        default:
                            break;
                    }
                }
                break;
            default:
                break;
        }

        switch(kop_work) {
            case 0:
            case 1:
            case 2:
            case 6:
            case 7:
                {
                    switch(fop_work) {
                        case 3:
                        case 4:
                        case 5:
                        case 9:
                            // cdb_ap
                            cdb_set("$wl_bss_role1", "2");
                            cdb_set("$wl_bss_enable1", "1");
                            break;
                        default:
                            break;
                    }
                }
                break;
            case 3:
            case 9:
                {
                    switch(fop_work) {
                        case 0:
                        case 1:
                        case 2:
                        case 4:
                        case 5:
                        case 6:
                        case 7:
                            // cdb_ap_sta 1
                            cdb_set("$wl_bss_role1", "2");
                            cdb_set("$wl_bss_role2", "1");
                            cdb_set("$wl_bss_enable1", "1");
                            cdb_set("$wl_bss_enable2", "1");
                            break;
                        default:
                            break;
                    }
                }
                break;
            case 4:
            case 5:
                {
                    switch(fop_work) {
                        case 0:
                        case 1:
                        case 2:
                        case 3:
                        case 6:
                        case 7:
                            // cdb_ap_sta 257
                            cdb_set("$wl_bss_role1", "2");
                            cdb_set("$wl_bss_role2", "257");
                            cdb_set("$wl_bss_enable1", "1");
                            cdb_set("$wl_bss_enable2", "1");
                            break;
                        default:
                            break;
                    }
                }
                break;
            default:
                break;
        }
        done = 1;
    }
}

void clear_checkup_state(void)
{
    montage_checkup_t *ptbl = checkup_tbl;
    int tblcnt = sizeof(checkup_tbl) / sizeof(montage_checkup_t);
    for (--tblcnt;tblcnt >= 0; tblcnt--) {
        ptbl[tblcnt].exec_proc(0, 0, 1);
    }
}

int fcdb_get(const char *cname, char *fcdb)
{
    char buf[BUF_SIZE];
    char cdb[BUF_SIZE];
    FILE *fp;
    int  len;
    int  ret = EXIT_FAILURE;
    strcpy(cdb, cname);
    strcat(cdb, "=");
    len = strlen(cdb);
    if( f_exists(FCDB_FILE) ) {
        if ((fp = fopen(FCDB_FILE, "r"))) {
            while ( new_fgets(buf, sizeof(buf), fp) != NULL ) {
                if (!strncmp(buf, cdb, len)) {
                    if (buf[len] == '\'')
                        len++;
                    strcpy(fcdb, &buf[len]);
                    len = strlen(fcdb);
                    if (fcdb[len-1] == '\'')
                        fcdb[len-1] = '\0';
                    ret = EXIT_SUCCESS;
                    break;
                }
            }
            fclose(fp);
        }
    }
    return ret;
}

int checkup_main(int cdbsave, int needreboot)
{
    montage_checkup_t *ptbl = checkup_tbl;
    int tblcnt = sizeof(checkup_tbl) / sizeof(montage_checkup_t);
    char kcdb[BUF_SIZE];
    char fcdb[BUF_SIZE];
    int i;

    iscdbsave = cdbsave;
    isneedreboot = needreboot;

    for (i = 0; i < tblcnt; i++) {
        cdb_get((char *)ptbl[i].cname, kcdb);
        fcdb_get(ptbl[i].cname, fcdb);
        if (strcmp(kcdb, fcdb)) {
            ptbl[i].exec_proc(kcdb, fcdb, 0);
        }
    }
    clear_checkup_state();
    if (!iscdbsave && isneedreboot) {
        cdb_commit();
        reboot(RB_AUTOBOOT);
    }

    return 0;
}

