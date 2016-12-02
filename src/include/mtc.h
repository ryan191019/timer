
/**
 * @file mtc.h
 *
 * @author
 * @date    2015-10-06
 */

#ifndef _MTC_H_
#define _MTC_H_

#include <stdio.h>
#include <stdarg.h>
#include <stdlib.h>

#include <unistd.h>
#include <sys/types.h>
#include <signal.h>

#include <cdb.h>
#include <shutils.h>
#include <strutils.h>
#include <netutils.h>

#include "include/mtc_types.h"

#define MTC_PID "/var/run/mtc.pid"
#define UNKNOWN (-1)

typedef enum
{
    opMode_db       = 0,
    opMode_ap       = 1,
    opMode_wr       = 2,
    opMode_wi       = 3,
    opMode_rp       = 4,
    opMode_br       = 5,
    opMode_smart    = 6,
    opMode_mb       = 7,
    opMode_p2p      = 8,
    opMode_client   = 9,
    opMode_smartcfg = 99,
}OPMode;

typedef enum
{
    wanMode_unknown = 0,
    wanMode_static  = 1,
    wanMode_dhcp    = 2,
    wanMode_pppoe   = 3,
    wanMode_pptp    = 4,
    wanMode_l2tp    = 5,
    wanMode_bigpond = 6,
    wanMode_pppoe2  = 7,
    wanMode_pptp2   = 8,
    wanMode_3g      = 9,
}WANMode;

typedef enum
{
    wlRole_NONE     = 0,
    wlRole_STA      = 1,
    wlRole_AP       = 2,
}WLRole;

typedef enum
{
    semWin = 0,
    semFSMData,
    semTimer,
    semCfg,

    MAX_SEM_LIST
}SemList;

typedef struct {
    char MAC0[18];
    char MAC1[18];
    char MAC2[18];
    int misc;
}MtcBoot;

typedef struct {
    int op_work_mode;
    int wan_mode;
}MtcCdb;

typedef struct {
    char LAN[10];
    char WAN[10];
    char WANB[10];
    char STA[10];
    char AP[10];
    char BRSTA[10];
    char BRAP[10];
    char IFPLUGD[10];
    char LMAC[18];
    char WMAC[18];
    char STAMAC[18];
    int HOSTAPD;
    int WPASUP;
    int OMODE; // op_work_mode
    int WMODE; // wan_mode
}MtcRule;

typedef struct {
    MtcBoot boot;
    MtcCdb  cdb;
    MtcRule rule;
}MtcData;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

void mtc_lock(SemList lockname);
void mtc_unlock(SemList lockname);
int mtc_get_semid(SemList lockname);
int read_pid_file(char *file);

#endif
