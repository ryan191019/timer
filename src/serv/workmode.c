#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>

#include "include/mtc.h"
#include "include/mtc_time.h"
#include "include/mtc_misc.h"
#include "include/mtc_proc.h"

/*
# Working FILE
#    HOSTAPD=1       # for wlan script.      hostapd bin should use
#    WPASUP=1        # for wlan script.      wpa_supplicant bin should use
#    AP=$AP          # for wlan script.      WiFi AP interface
#    STA=$STA        # for wlan script.      WiFi STA interface
#    IFPLUGD         # for if script.        monitor interface of wan
#    MODE            # for if script.        current wan mode
#    OP              # for if script.        current work mode
#                                            0: debug mode
#                                            1: AP router(disable nat/dhcp)
#                                            2: Wifi router
#                                            3: WISP mode
#                                            4: Repeater mode(disable nat/dhcp)
#                                            5: Bridge mode(disable nat/dhcp)
#                                            6: Smart WAN
#                                            7: Mobile 3G
#                                            8: P2P
#                                            9: Pure Client mode
*/

#define PROCETH "/proc/eth"
#define PROCWLA "/proc/wla"

#define VIDW     "0"
#define VIDL     "4095"
#define INF_ETHW "eth0.0"
#define INF_ETHL "eth0.4095"
#define INF_AP   "wlan0"
#define INF_STA  "sta0"
#define INF_BR0  "br0"
#define INF_BR1  "br1"

#define IFPLUGD_NAME             "ifplugd"
#define IFPLUGD_PROC             "/usr/bin/"IFPLUGD_NAME
#define IFPLUGD_ETHL_PIDFILE     "/var/run/"IFPLUGD_NAME"."INF_ETHL".pid"
#define IFPLUGD_ETHW_PIDFILE     "/var/run/"IFPLUGD_NAME"."INF_ETHW".pid"

extern MtcData *mtc;

static int stop_br_probe = 0;

static void do_mac_init(void)
{
    unsigned char val[9];
    char wan_clone_mac[MBUF_LEN];
    int wan_clone_mac_enable = cdb_get_int("$wan_clone_mac_enable", 0);

    if ((wan_clone_mac_enable == 1) && 
        (cdb_get_str("$wan_clone_mac", wan_clone_mac, sizeof(wan_clone_mac), NULL) != NULL)) {
        my_mac2val(val, wan_clone_mac);
        my_mac2val(val+3, mtc->boot.MAC0);
        my_val2mac(mtc->rule.LMAC, val);
        sprintf(mtc->rule.WMAC, "%s", wan_clone_mac);
        if (mtc->cdb.op_work_mode == 3)
            sprintf(mtc->rule.STAMAC, "%s", wan_clone_mac);
        else {
            my_mac2val(val+3, mtc->boot.MAC2);
            my_val2mac(mtc->rule.STAMAC, val);
        }
    }
    else {
        sprintf(mtc->rule.LMAC, "%s", mtc->boot.MAC0);
        sprintf(mtc->rule.WMAC, "%s", mtc->boot.MAC1);
        sprintf(mtc->rule.STAMAC, "%s", mtc->boot.MAC2);
    }
}

static void do_lan_mac(void)
{
    exec_cmd2("ifconfig %s hw ether %s", INF_BR0, mtc->rule.LMAC);
    exec_cmd2("ifconfig eth0 hw ether %s", mtc->rule.LMAC);
    exec_cmd2("ifconfig %s hw ether %s", INF_AP, mtc->rule.LMAC);
}

static void do_forward_mode(void)
{
    cdb_set_int("$wl_forward_mode", cdb_get_int("$wl_def_forward_mode", 0));
}

static void do_eth_reload(int yes)
{
    exec_cmd2("echo sclk > %s", PROCETH);
    if (yes == 1) {
        exec_cmd2("echo 8021q 0fff000b > %s", PROCETH);
        exec_cmd2("echo 8021q 00008100 > %s", PROCETH);
        exec_cmd2("echo bssid 0fff0000 > %s", PROCETH);
        exec_cmd2("echo epvid 0fff0100 > %s", PROCETH);
        exec_cmd2("echo epvid 0fff0200 > %s", PROCETH);
        exec_cmd2("echo epvid 0fff0000 > %s", PROCETH);
        exec_cmd2("echo wmode 0 > %s", PROCETH);
        exec_cmd2("echo swctl 1 > %s", PROCETH);
    }
    else if (yes == 0) {
        exec_cmd2("echo 8021q 0fff000e > %s", PROCETH);
        exec_cmd2("echo 8021q 00008105 > %s", PROCETH);
        exec_cmd2("echo epvid 0fff0100 > %s", PROCETH);
        exec_cmd2("echo epvid 00000000 > %s", PROCETH);
        exec_cmd2("echo epvid 0fff0200 > %s", PROCETH);
        exec_cmd2("echo bssid 0fff0000 > %s", PROCETH);
        exec_cmd2("echo wmode 0 > %s", PROCETH);
        exec_cmd2("echo swctl 1 > %s", PROCETH);
    }
}

static void do_br_probe(int sData, void *data)
{
    char st[BUF_LEN_16] = { 0 };
    char br[BUF_LEN_16] = { 0 };
    FILE *fp;

    if ((mtc->rule.OMODE == opMode_db) && !stop_br_probe) {
        if((fp = _system_read("cat /sys/kernel/debug/eth/br_state"))) {
            new_fgets(st, sizeof(st), fp);
            _system_close(fp);
        }
        if((fp = _system_read("cat /sys/kernel/debug/eth/br"))) {
            new_fgets(br, sizeof(br), fp);
            _system_close(fp);
        }
        if ((st[0] == '0') && (br[0] == '1')) {
             do_eth_reload(1);
        }
        if ((st[0] == '1') && (br[0] == '0')) {
             do_eth_reload(0);
        }
        if ((br[0] == '0') || (br[0] == '1')) {
            exec_cmd2("echo %c > /sys/kernel/debug/eth/br_state", br[0]);
        }

        mtcTimerMod(timerMtcBrProbe, 5);
    }
}

static void do_db_mode(void)
{
    // Forwarding mode
    do_forward_mode();

    cdb_set_int("$wl_bss_role2", 0);

    // Bridge
    exec_cmd2("brctl addbr %s", INF_BR0);

    // Vlan interface
    exec_cmd2("vconfig add eth0 %s", VIDL);
    exec_cmd2("vconfig add eth0 %s", VIDW);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Bring up eth interface to do calibration
    exec_cmd2("ifconfig %s up", INF_ETHL);
    exec_cmd2("ifconfig %s up", INF_ETHW);
    exec_cmd2("ifconfig eth0 up");
    exec_cmd2("ifconfig %s up", INF_BR0);

    // Config mac address
    do_lan_mac();

    // Config mac address
    exec_cmd2("ifconfig %s hw ether %s", INF_ETHW, mtc->rule.WMAC);

    // Generate rule
    strcpy(mtc->rule.LAN, INF_BR0);
    strcpy(mtc->rule.WAN, "");
    strcpy(mtc->rule.WANB, "");
    strcpy(mtc->rule.AP, INF_AP);
    strcpy(mtc->rule.BRAP, INF_BR0);
    strcpy(mtc->rule.IFPLUGD, INF_ETHW);
    mtc->rule.HOSTAPD = 1;
    mtc->rule.WPASUP = 0;
    mtc->rule.OMODE = mtc->cdb.op_work_mode;
    mtc->rule.WMODE = mtc->cdb.wan_mode;

    do_eth_reload(0);

    exec_cmd2("start-stop-daemon -S -p %s -x %s -- -u 0 -d 0 -p -q -i %s", 
              IFPLUGD_ETHL_PIDFILE, IFPLUGD_PROC, INF_ETHL);
    exec_cmd2("start-stop-daemon -S -p %s -x %s -- -u 0 -d 0 -p -q -i %s", 
              IFPLUGD_ETHW_PIDFILE, IFPLUGD_PROC, INF_ETHW);

    exec_cmd2("echo mdio 0 0 3100 > %s", PROCETH);
    mtcTimerAdd(timerMtcBrProbe, do_br_probe, 0, NULL, 0);
}

static void do_ap_mode(void)
{
    // Forwarding mode
    do_forward_mode();

    cdb_set_int("$wl_bss_role2", 0);

    // Bridge
    exec_cmd2("brctl addbr %s", INF_BR0);

    // Vlan interface
    exec_cmd2("vconfig add eth0 %s", VIDL);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Bring up eth interface to do calibration
    exec_cmd2("ifconfig %s up", INF_ETHL);
    exec_cmd2("ifconfig eth0 up");
    exec_cmd2("ifconfig %s up", INF_BR0);

    // Config mac address
    do_lan_mac();

    // Generate rule
    strcpy(mtc->rule.LAN, INF_BR0);
    strcpy(mtc->rule.WAN, "");
    strcpy(mtc->rule.WANB, "");
    strcpy(mtc->rule.AP, INF_AP);
    strcpy(mtc->rule.BRAP, INF_BR0);
    strcpy(mtc->rule.IFPLUGD, "");
    mtc->rule.HOSTAPD = 1;
    mtc->rule.WPASUP = 0;
    mtc->rule.OMODE = mtc->cdb.op_work_mode;
    mtc->rule.WMODE = mtc->cdb.wan_mode;

    exec_cmd2("echo sclk > %s", PROCETH);
#if defined(CONFIG_P0_AS_LAN)
    exec_cmd2("echo 8021q 0fff000d > %s", PROCETH);
    exec_cmd2("echo epvid 0fff0000 > %s", PROCETH);
#else
    exec_cmd2("echo 8021q 0fff000e > %s", PROCETH);
    exec_cmd2("echo epvid 0fff0100 > %s", PROCETH);
#endif
    exec_cmd2("echo epvid 0fff0200 > %s", PROCETH);
    exec_cmd2("echo bssid 0fff0000 > %s", PROCETH);
    exec_cmd2("echo wmode %d > %s", mtc->cdb.op_work_mode, PROCETH);
    exec_cmd2("echo swctl 1 > %s", PROCETH);

    exec_cmd2("start-stop-daemon -S -p %s -x %s -- -u 0 -d 0 -p -q -i %s", 
              IFPLUGD_ETHL_PIDFILE, IFPLUGD_PROC, INF_ETHL);
}

static void do_wr_mode(void)
{
    // Forwarding mode
    do_forward_mode();

    cdb_set_int("$wl_bss_role2", 0);

    // Bridge
    exec_cmd2("brctl addbr %s", INF_BR0);

    // Vlan interface
    exec_cmd2("vconfig add eth0 %s", VIDL);
    exec_cmd2("vconfig add eth0 %s", VIDW);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Bring up eth interface to do calibration
    exec_cmd2("ifconfig %s up", INF_ETHL);
    exec_cmd2("ifconfig %s up", INF_ETHW);
    exec_cmd2("ifconfig eth0 up");
    exec_cmd2("ifconfig %s up", INF_BR0);

    // Config mac address
    do_lan_mac();

    // Config mac address
    exec_cmd2("ifconfig %s hw ether %s", INF_ETHW, mtc->rule.WMAC);

    // Generate rule
    strcpy(mtc->rule.LAN, INF_BR0);
    strcpy(mtc->rule.WAN, "");
    strcpy(mtc->rule.WANB, "");
    strcpy(mtc->rule.AP, INF_AP);
    strcpy(mtc->rule.BRAP, INF_BR0);
    strcpy(mtc->rule.IFPLUGD, INF_ETHW);
    mtc->rule.HOSTAPD = 1;
    mtc->rule.WPASUP = 0;
    mtc->rule.OMODE = mtc->cdb.op_work_mode;
    mtc->rule.WMODE = mtc->cdb.wan_mode;

    exec_cmd2("echo sclk > %s", PROCETH);
    exec_cmd2("echo 8021q 0fff000c > %s", PROCETH);
#if defined(CONFIG_P0_AS_LAN)
    exec_cmd2("echo 8021q 00008105 > %s", PROCETH);
    exec_cmd2("echo epvid 00000000 > %s", PROCETH);
#else
    exec_cmd2("echo 8021q 00008106 > %s", PROCETH);
    exec_cmd2("echo epvid 00000100 > %s", PROCETH);
#endif
    exec_cmd2("echo epvid 0fff0200 > %s", PROCETH);
    exec_cmd2("echo bssid 0fff0000 > %s", PROCETH);
    exec_cmd2("echo wmode %d > %s", mtc->cdb.op_work_mode, PROCETH);
    exec_cmd2("echo swctl 1 > %s", PROCETH);

    exec_cmd2("start-stop-daemon -S -p %s -x %s -- -u 0 -d 0 -p -q -i %s", 
              IFPLUGD_ETHW_PIDFILE, IFPLUGD_PROC, INF_ETHW);
}

static void do_wi_mode(void)
{
    // Forwarding mode
    do_forward_mode();

    cdb_set_int("$wl_bss_role2", 0);

    // Bridge
    exec_cmd2("brctl addbr %s", INF_BR0);
    exec_cmd2("brctl addbr %s -NO_FLOOD_MC -FLOOD_UNKNOW_UC", INF_BR1);

    // Vlan interface
    exec_cmd2("vconfig add eth0 %s", VIDL);
    exec_cmd2("vconfig add eth0 %s", VIDW);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Add ether iface in br1
    exec_cmd2("brctl addif %s %s", INF_BR1, INF_ETHW);

    // Bring up eth interface to do calibration
    exec_cmd2("ifconfig %s up", INF_ETHL);
    exec_cmd2("ifconfig %s up", INF_ETHW);
    exec_cmd2("ifconfig eth0 up");
    exec_cmd2("ifconfig %s up", INF_BR0);
    exec_cmd2("ifconfig %s up", INF_BR1);

    // Config mac address
    do_lan_mac();

    // Config mac address
    exec_cmd2("ifconfig %s hw ether %s", INF_BR1, mtc->rule.STAMAC);
    exec_cmd2("ifconfig %s hw ether %s", INF_ETHW, mtc->rule.STAMAC);

    // Generate rule
    strcpy(mtc->rule.LAN, INF_BR0);
    strcpy(mtc->rule.WAN, "");
    strcpy(mtc->rule.WANB, "");
    strcpy(mtc->rule.AP, INF_AP);
    strcpy(mtc->rule.STA, INF_STA);
    strcpy(mtc->rule.BRAP, INF_BR0);
    strcpy(mtc->rule.BRSTA, INF_BR1);
    strcpy(mtc->rule.IFPLUGD, INF_STA);
    mtc->rule.HOSTAPD = 1;
    mtc->rule.WPASUP = 1;
    mtc->rule.OMODE = mtc->cdb.op_work_mode;
    mtc->rule.WMODE = mtc->cdb.wan_mode;

    exec_cmd2("echo sclk > %s", PROCETH);
    exec_cmd2("echo 8021q 0000810c > %s", PROCETH);
#if defined(CONFIG_P0_AS_LAN)
    exec_cmd2("echo 8021q 0fff000d > %s", PROCETH);
    exec_cmd2("echo epvid 0fff0000 > %s", PROCETH);
#else
    exec_cmd2("echo 8021q 0fff000e > %s", PROCETH);
    exec_cmd2("echo epvid 0fff0100 > %s", PROCETH);
#endif
    exec_cmd2("echo epvid 0fff0200 > %s", PROCETH);
    exec_cmd2("echo bssid 0fff0000 > %s", PROCETH);
    exec_cmd2("echo bssid 00000100 > %s", PROCETH);
    exec_cmd2("echo wmode %d > %s", mtc->cdb.op_work_mode, PROCETH);
    exec_cmd2("echo swctl 1 > %s", PROCETH);

    exec_cmd2("start-stop-daemon -S -p %s -x %s -- -u 0 -d 0 -p -q -i %s", 
              IFPLUGD_ETHL_PIDFILE, IFPLUGD_PROC, INF_ETHL);
}

static void do_br_mode(void)
{
    // SW Forwarding mode
    cdb_set_int("$wl_forward_mode", 1);

    cdb_set_int("$wl_bss_role2", 256);

    // Bridge
    exec_cmd2("brctl addbr %s -FLOOD_UNKNOW_UC", INF_BR0);

    // Vlan interface
    exec_cmd2("vconfig add eth0 %s", VIDL);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Bring up eth interface to do calibration
    exec_cmd2("ifconfig %s up", INF_ETHL);
    exec_cmd2("ifconfig eth0 up");
    exec_cmd2("ifconfig %s up", INF_BR0);

    // Config mac address
    do_lan_mac();

    // Config AP unit same as STA
    exec_cmd2("echo config.apid=1 > %s", PROCWLA);
    exec_cmd2("echo config.staid=1 > %s", PROCWLA);

    // Generate rule
    strcpy(mtc->rule.LAN, INF_BR0);
    strcpy(mtc->rule.WAN, INF_BR0);
    strcpy(mtc->rule.WANB, "");
    strcpy(mtc->rule.AP, INF_AP);
    strcpy(mtc->rule.STA, INF_STA);
    strcpy(mtc->rule.BRAP, INF_BR0);
    strcpy(mtc->rule.BRSTA, INF_BR0);
    strcpy(mtc->rule.IFPLUGD, INF_STA);
    mtc->rule.HOSTAPD = 1;
    mtc->rule.WPASUP = 1;
    mtc->rule.OMODE = mtc->cdb.op_work_mode;
    mtc->rule.WMODE = mtc->cdb.wan_mode;

    exec_cmd2("echo sclk > %s", PROCETH);
#if defined(CONFIG_P0_AS_LAN)
    exec_cmd2("echo 8021q 0fff000d > %s", PROCETH);
    exec_cmd2("echo epvid 0fff0000 > %s", PROCETH);
#else
    exec_cmd2("echo 8021q 0fff000e > %s", PROCETH);
    exec_cmd2("echo epvid 0fff0100 > %s", PROCETH);
#endif
    exec_cmd2("echo epvid 0fff0200 > %s", PROCETH);
    exec_cmd2("echo bssid 0fff0000 > %s", PROCETH);
    exec_cmd2("echo wmode %d > %s", mtc->cdb.op_work_mode, PROCETH);
    exec_cmd2("echo swctl 1 > %s", PROCETH);

    exec_cmd2("start-stop-daemon -S -p %s -x %s -- -u 0 -d 0 -p -q -i %s", 
              IFPLUGD_ETHL_PIDFILE, IFPLUGD_PROC, INF_ETHL);
}

static void do_smart_mode(void)
{
    if ((mtc->cdb.wan_mode == wanMode_dhcp) || (mtc->cdb.wan_mode == wanMode_pppoe)) {
        do_wr_mode();
    }
    else if (mtc->cdb.wan_mode == wanMode_3g) {
        do_ap_mode();
    }
}

static void do_mb_mode(void)
{
    do_ap_mode();
}

static void do_p2p_mode(void)
{
    // Bridge
    exec_cmd2("brctl addbr %s", INF_BR0);
    exec_cmd2("brctl addbr %s -NO_FLOOD_MC -FLOOD_UNKNOW_UC", INF_BR1);

    // Vlan interface
    exec_cmd2("vconfig add eth0 %s", VIDL);
    exec_cmd2("vconfig add eth0 %s", VIDW);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Add ether iface in br1
    exec_cmd2("brctl addif %s %s", INF_BR1, INF_ETHW);

    // Bring up eth interface to do calibration
    exec_cmd2("ifconfig %s up", INF_ETHL);
    exec_cmd2("ifconfig %s up", INF_ETHW);
    exec_cmd2("ifconfig eth0 up");
    exec_cmd2("ifconfig %s up", INF_BR0);
    exec_cmd2("ifconfig %s up", INF_BR1);

    // Config mac address
    do_lan_mac();

    // Config mac address
    exec_cmd2("ifconfig %s hw ether %s", INF_BR1, mtc->rule.LMAC);
    exec_cmd2("ifconfig %s hw ether %s", INF_ETHW, mtc->rule.LMAC);

    // Generate rule
    strcpy(mtc->rule.LAN, INF_BR0);
    strcpy(mtc->rule.WAN, "");
    strcpy(mtc->rule.WANB, "");
    strcpy(mtc->rule.AP, "");
    strcpy(mtc->rule.STA, INF_AP);
    strcpy(mtc->rule.BRAP, INF_BR0);
    strcpy(mtc->rule.BRSTA, INF_BR1);
    strcpy(mtc->rule.IFPLUGD, INF_BR1);
    mtc->rule.HOSTAPD = 0;
    mtc->rule.WPASUP = 1;
    mtc->rule.OMODE = mtc->cdb.op_work_mode;
    mtc->rule.WMODE = mtc->cdb.wan_mode;

    exec_cmd2("echo sclk > %s", PROCETH);
    exec_cmd2("echo 8021q 0000810c > %s", PROCETH);
#if defined(CONFIG_P0_AS_LAN)
    exec_cmd2("echo 8021q 0fff000d > %s", PROCETH);
    exec_cmd2("echo epvid 0fff0000 > %s", PROCETH);
#else
    exec_cmd2("echo 8021q 0fff000e > %s", PROCETH);
    exec_cmd2("echo epvid 0fff0100 > %s", PROCETH);
#endif
    exec_cmd2("echo epvid 0fff0200 > %s", PROCETH);
    exec_cmd2("echo bssid 0fff0000 > %s", PROCETH);
    exec_cmd2("echo bssid 00000100 > %s", PROCETH);
    exec_cmd2("echo wmode %d > %s", mtc->cdb.op_work_mode, PROCETH);
    exec_cmd2("echo swctl 1 > %s", PROCETH);

    exec_cmd2("start-stop-daemon -S -p %s -x %s -- -u 0 -d 0 -p -q -i %s", 
              IFPLUGD_ETHW_PIDFILE, IFPLUGD_PROC, INF_ETHW);
}

static void do_client_mode(void)
{
    // Forwarding mode
    do_forward_mode();

    cdb_set_int("$wl_bss_role2", 0);

    // Bridge
#if defined(CONFIG_STA_MODE_BR0_SUPPORT)
    exec_cmd2("brctl addbr %s", INF_BR0);
#endif
    exec_cmd2("brctl addbr %s -NO_FLOOD_MC -FLOOD_UNKNOW_UC", INF_BR1);

    // Vlan interface
#if defined(CONFIG_STA_MODE_BR0_SUPPORT)
    exec_cmd2("vconfig add eth0 %s", VIDL);
#endif
    exec_cmd2("vconfig add eth0 %s", VIDW);

    // Add ether iface in br0
#if defined(CONFIG_STA_MODE_BR0_SUPPORT)
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);
#endif
    exec_cmd2("brctl addif %s %s", INF_BR1, INF_ETHW);

    // Bring up eth interface to do calibration
#if defined(CONFIG_STA_MODE_BR0_SUPPORT)
    exec_cmd2("ifconfig %s up", INF_ETHL);
#endif
    exec_cmd2("ifconfig %s up", INF_ETHW);
    exec_cmd2("ifconfig eth0 up");
#if defined(CONFIG_STA_MODE_BR0_SUPPORT)
    exec_cmd2("ifconfig %s up", INF_BR0);
#endif
    exec_cmd2("ifconfig %s up", INF_BR1);

    // Config mac address
    do_lan_mac();

    // Config mac address
    exec_cmd2("ifconfig %s hw ether %s", INF_BR1, mtc->rule.STAMAC);
    exec_cmd2("ifconfig %s hw ether %s", INF_ETHW, mtc->rule.STAMAC);

    // Generate rule
#if defined(CONFIG_STA_MODE_BR0_SUPPORT)
    strcpy(mtc->rule.LAN, INF_BR0);
#else
    strcpy(mtc->rule.LAN, "");
#endif
    strcpy(mtc->rule.WAN, "");
    strcpy(mtc->rule.WANB, "");
    strcpy(mtc->rule.AP, "");
    strcpy(mtc->rule.STA, INF_STA);
    strcpy(mtc->rule.BRAP, "");
    strcpy(mtc->rule.BRSTA, INF_BR1);
    strcpy(mtc->rule.IFPLUGD, INF_STA);
    mtc->rule.HOSTAPD = 0;
    mtc->rule.WPASUP = 1;
    mtc->rule.OMODE = mtc->cdb.op_work_mode;
    mtc->rule.WMODE = mtc->cdb.wan_mode;

    exec_cmd2("echo sclk > %s", PROCETH);
    exec_cmd2("echo 8021q 0000810c > %s", PROCETH);
#if defined(CONFIG_P0_AS_LAN)
    exec_cmd2("echo 8021q 0fff000d > %s", PROCETH);
    exec_cmd2("echo epvid 0fff0000 > %s", PROCETH);
#else
    exec_cmd2("echo 8021q 0fff000e > %s", PROCETH);
    exec_cmd2("echo epvid 0fff0100 > %s", PROCETH);
#endif
    exec_cmd2("echo epvid 0fff0200 > %s", PROCETH);
    exec_cmd2("echo bssid 0fff0000 > %s", PROCETH);
    exec_cmd2("echo bssid 00000100 > %s", PROCETH);
    exec_cmd2("echo wmode %d > %s", mtc->cdb.op_work_mode, PROCETH);
    exec_cmd2("echo swctl 1 > %s", PROCETH);

    exec_cmd2("start-stop-daemon -S -p %s -x %s -- -u 0 -d 0 -p -q -i %s", 
              IFPLUGD_ETHL_PIDFILE, IFPLUGD_PROC, INF_ETHL);
}

static void do_smartcfg_mode(void)
{
    // Bridge
    exec_cmd2("brctl addbr %s", INF_BR0);

    // Vlan interface
    exec_cmd2("vconfig add eth0 %s", VIDL);

    // Add ether iface in br0
    exec_cmd2("brctl addif %s %s", INF_BR0, INF_ETHL);

    // Bring up eth interface to do calibration
    exec_cmd2("ifconfig %s up", INF_ETHL);
    exec_cmd2("ifconfig eth0 up");
    exec_cmd2("ifconfig %s up", INF_BR0);

    // Config mac address
    do_lan_mac();

    exec_cmd2("echo config.smart_config=1 > %s", PROCWLA);

    // Generate rule
    strcpy(mtc->rule.LAN, "");
    strcpy(mtc->rule.WAN, "");
    strcpy(mtc->rule.WANB, "");
    strcpy(mtc->rule.AP, "");
    strcpy(mtc->rule.STA, INF_STA);
    strcpy(mtc->rule.BRAP, "");
    strcpy(mtc->rule.BRSTA, "");
    strcpy(mtc->rule.IFPLUGD, INF_STA);
    mtc->rule.HOSTAPD = 0;
    mtc->rule.WPASUP = 0;
    mtc->rule.OMODE = mtc->cdb.op_work_mode;
    mtc->rule.WMODE = mtc->cdb.wan_mode;

    exec_cmd2("echo sclk > %s", PROCETH);
#if defined(CONFIG_P0_AS_LAN)
    exec_cmd2("echo 8021q 0fff000d > %s", PROCETH);
    exec_cmd2("echo epvid 0fff0000 > %s", PROCETH);
#else
    exec_cmd2("echo 8021q 0fff000e > %s", PROCETH);
    exec_cmd2("echo epvid 0fff0100 > %s", PROCETH);
#endif
    exec_cmd2("echo epvid 0fff0200 > %s", PROCETH);
    exec_cmd2("echo bssid 0fff0000 > %s", PROCETH);
    exec_cmd2("echo wmode %d > %s", mtc->cdb.op_work_mode, PROCETH);
    exec_cmd2("echo swctl 1 > %s", PROCETH);

    exec_cmd2("start-stop-daemon -S -p %s -x %s -- -u 0 -d 0 -p -q -i %s", 
              IFPLUGD_ETHL_PIDFILE, IFPLUGD_PROC, INF_ETHL);
}

static int start(void)
{
    int omod, wmod;
    char usbif_find_eth[SSBUF_LEN] = {0};

    mtc->cdb.op_work_mode = cdb_get_int("$op_work_mode", opMode_db);
    mtc->cdb.wan_mode = cdb_get_int("$wan_mode", wanMode_dhcp);

    omod = mtc->cdb.op_work_mode;
    wmod = mtc->cdb.wan_mode;

    do_mac_init();

    if (cdb_get_int("$smrt_enable", 0) == 1)
        mtc->cdb.op_work_mode = opMode_smartcfg;

    MTC_LOGGER(this, LOG_INFO, "LMAC  =%s", mtc->rule.LMAC);
    MTC_LOGGER(this, LOG_INFO, "WMAC  =%s", mtc->rule.WMAC);
    MTC_LOGGER(this, LOG_INFO, "STAMAC=%s", mtc->rule.STAMAC);

    switch(mtc->cdb.op_work_mode) {
        case opMode_db:
            do_db_mode();
            break;
        case opMode_ap:
            do_ap_mode();
            break;
        case opMode_wr:
            do_wr_mode();
            break;
        case opMode_wi:
            do_wi_mode();
            break;
        case opMode_rp:
            do_br_mode();
            break;
        case opMode_br:
            do_br_mode();
            break;
        case opMode_smart:
            do_smart_mode();
            break;
        case opMode_mb:
            do_mb_mode();
            break;
        case opMode_p2p:
            do_p2p_mode();
            break;
        case opMode_client:
            do_client_mode();
            break;
        case opMode_smartcfg:
            do_smartcfg_mode();
            break;
        default:
            break;
    }

    if (cdb_get_int("$smrt_enable", 0) == 1)
        mtc->cdb.op_work_mode = opMode_smartcfg;

    if(wmod == 1 || wmod == 2) {
       if(omod == 0 || omod == 2 || omod == 6) {
          strcpy(mtc->rule.WAN, "eth0.0");
          strcpy(mtc->rule.WANB, "eth0.0");
       }
       else if(omod == 4 || omod == 5) {
          strcpy(mtc->rule.WAN, "br0");
          strcpy(mtc->rule.WANB, "br0");
       }
       else if(omod == 3 || omod == 9) {
          strcpy(mtc->rule.WAN, "br1");
          strcpy(mtc->rule.WANB, "br1");
       }
       else if(omod == 1) {
          strcpy(mtc->rule.WAN, "");
          strcpy(mtc->rule.WANB, "");
       }
    }
    else if(wmod == 3 || wmod == 4 || wmod == 5 || wmod == 7 || wmod == 8) {
       if(omod == 0 || omod == 2 || omod == 6) {
          strcpy(mtc->rule.WAN, "ppp0");
          strcpy(mtc->rule.WANB, "eth0.0");
       }
       else if(omod == 3) {
          strcpy(mtc->rule.WAN, "ppp0");
          strcpy(mtc->rule.WANB, "br1");
       }
       else {
          strcpy(mtc->rule.WAN, "ppp0");
          strcpy(mtc->rule.WANB, "ppp0");
       }
    }
    else if(wmod == 9) {
       if(cdb_get_str("$usbif_find_eth", usbif_find_eth, sizeof(usbif_find_eth), NULL) != NULL) {
          strcpy(mtc->rule.WAN, usbif_find_eth);
          strcpy(mtc->rule.WANB, usbif_find_eth);
       }
       else {
          strcpy(mtc->rule.WAN, "ppp0");
          strcpy(mtc->rule.WANB, "ppp0");
       }
    }

    if(omod == 1) {
       if(wmod == 9) {
          if(cdb_get_str("$usbif_find_eth", usbif_find_eth, sizeof(usbif_find_eth), NULL) != NULL) {
             strcpy(mtc->rule.WAN, usbif_find_eth);
             strcpy(mtc->rule.WANB, usbif_find_eth);
          }
          else {
             strcpy(mtc->rule.WAN, "ppp0");
             strcpy(mtc->rule.WANB, "ppp0");
          }
       }
       else {
          strcpy(mtc->rule.WAN, "");
          strcpy(mtc->rule.WANB, "");
       }
    }

    MTC_LOGGER(this, LOG_INFO, "workmode start: attach ifplugd op(%d)", mtc->cdb.op_work_mode);

    return 0;
}

static int stop(void)
{
    exec_cmd2("echo swctl 0 > %s", PROCETH);
    exec_cmd2("killall -s 9 %s", IFPLUGD_NAME);
    unlink(IFPLUGD_ETHL_PIDFILE);
    unlink(IFPLUGD_ETHW_PIDFILE);
    stop_br_probe = 1;
    if (mtc->rule.OMODE == opMode_wi) {
        exec_cmd2("brctl delif %s %s", INF_BR1, INF_ETHW);
        exec_cmd2("ifconfig %s down", INF_BR1);
        exec_cmd2("brctl delbr %s", INF_BR1);
    }
    else if ((mtc->rule.OMODE == opMode_rp) || (mtc->rule.OMODE == opMode_br)) {
        exec_cmd2("brctl delif %s %s", INF_BR0, INF_STA);
    }

    // br_mode need the special br setting, remove the old one
    exec_cmd2("ifconfig %s down", INF_BR0);
    exec_cmd2("brctl delbr %s", INF_BR0);

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int workmode_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

