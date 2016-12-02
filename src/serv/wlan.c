#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

#include "include/mtc.h"
#include "include/mtc_misc.h"
#include "include/mtc_proc.h"

#define IFPLUGD_NAME             "ifplugd"
#define IFPLUGD_PROC             "/usr/bin/"IFPLUGD_NAME
#define IFPLUGD_STA0_PIDFILE     "/var/run/"IFPLUGD_NAME".sta0.pid"

extern MtcData *mtc;

//#define HAVE_MULTIPLE_BSSID

#define GPIO_PATH              "/sys/devices/platform/cta-gpio/gpio"

#define HOSTAPD_CLI_ACTIONFILE "/tmp/hostapd_cli_action"
#define HOSTAPD_CLI_PIDFILE    "/var/run/hostapd_cli.pid"
#define HOSTAPD_CLI_CMDLINE    "-a "HOSTAPD_CLI_ACTIONFILE
#define HOSTAPD_CLI_BIN        "/usr/sbin/hostapd_cli"

#define HOSTAPD_CFGFILE        "/var/run/hostapd.conf"
#define HOSTAPD_PIDFILE        "/var/run/hostapd.pid"
#define HOSTAPD_ACLFILE        "/var/run/hostapd.acl"
#define HOSTAPD_CMDLINE        "-B -P "HOSTAPD_PIDFILE" "HOSTAPD_CFGFILE
#define HOSTAPD_BIN            "/usr/sbin/hostapd"

#define WPASUP_CFGFILE         "/var/run/wpa.conf"
#define WPASUP_PIDFILE         "/var/run/wpa.pid"
#define WPASUP_HCTRL           "-H /var/run/hostapd/wlan0"
#define WPASUP_CMDLINE         "-B -P "WPASUP_PIDFILE" -i %s -Dnl80211 -c "WPASUP_CFGFILE
#define WPASUP_BIN             "/usr/sbin/wpa_supplicant"

#define P2P_PIDFILE            "/var/run/p2p.pid"

#define PROCWLA                "/proc/wla"

#define AUTH_CAP_NONE          (0)
#define AUTH_CAP_OPEN          (1)
#define AUTH_CAP_SHARED        (2)
#define AUTH_CAP_WEP           (4)
#define AUTH_CAP_WPA           (8)
#define AUTH_CAP_WPA2          (16)
#define AUTH_CAP_WPS           (32)
#define AUTH_CAP_WAPI          (128)

#define AUTH_CAP_CIPHER_NONE   (0)
#define AUTH_CAP_CIPHER_WEP40  (1)
#define AUTH_CAP_CIPHER_WEP104 (2)
#define AUTH_CAP_CIPHER_TKIP   (4)
#define AUTH_CAP_CIPHER_CCMP   (8)
#define AUTH_CAP_CIPHER_WAPI   (16)

#define BB_MODE_B              (1)
#define BB_MODE_G              (2)
#define BB_MODE_BG             (3)
#define BB_MODE_N              (4)
#define BB_MODE_BGN            (7)


//wds rule $wl_wds1='gkey=1&en=1&mac=00:12:0e:b5:5b:10&cipher=1&kidx=0'
static void config_wds(void)
{
    char wl_wds[MSBUF_LEN] = { 0 };
    char macaddr[MBUF_LEN] = { 0 };
    char wl_bss_wep_key[MSBUF_LEN] = { 0 };
    char wl_bss_wep_keyname[USBUF_LEN];
    char *ptr;
    char *argv[10];
    char *en, *mac, *cipher, *kidx;
    int num;
    int rule;
    int i, j;
    u32 idx;

    for (i=1, rule=0;;i++, rule++) {
        if (cdb_get_array_str("$wl_wds", i, wl_wds, sizeof(wl_wds), NULL) == NULL) {
            break;
        }
        else if (strlen(wl_wds) == 0) {
            break;
        }
        else {
            MTC_LOGGER(this, LOG_INFO, "%s: wl_wds%d is [%s]", __func__, i, wl_wds);

            num = str2argv(wl_wds, argv, '&');
            if((num < 5)                 || !str_arg(argv, "gkey=") || 
               !str_arg(argv, "en=")     || !str_arg(argv, "mac=")  || 
               !str_arg(argv, "cipher=") || !str_arg(argv, "kidx=")) {
                MTC_LOGGER(this, LOG_INFO, "%s: skip, argv is not complete", __func__);
                continue;
            }

            en     = str_arg(argv, "en=");
            mac    = str_arg(argv, "mac=");
            cipher = str_arg(argv, "cipher=");
            kidx   = str_arg(argv, "kidx=");

            MTC_LOGGER(this, LOG_INFO, "%s: en is [%s]", __func__, en);
            MTC_LOGGER(this, LOG_INFO, "%s: mac is [%s]", __func__, mac);
            MTC_LOGGER(this, LOG_INFO, "%s: cipher is [%s]", __func__, cipher);
            MTC_LOGGER(this, LOG_INFO, "%s: kidx is [%s]", __func__, kidx);

            for (j=0, ptr=mac; *ptr!=0; ptr++) {
                if (*ptr != ':') {
                    macaddr[j++] = *ptr;
                }
            }
            macaddr[j] = 0;
            exec_cmd2("echo \"config.wds_peer_addr.%d=0x%s\" > %s", rule, macaddr, PROCWLA);
            exec_cmd2("echo \"config.wds_cipher_type.%d=%s\" > %s", rule, cipher, PROCWLA);
            exec_cmd2("echo \"config.wds_keyid.%d=%s\" > %s", rule, kidx, PROCWLA);

            if (!strcmp(cipher, "1") || !strcmp(cipher, "2")) {
                idx = strtoul(kidx, NULL, 0) + 1; 
                sprintf(wl_bss_wep_keyname, "wl_bss_wep_%dkey", idx);
                if (cdb_get_array_str(wl_bss_wep_keyname, 
                    idx, wl_bss_wep_key, sizeof(wl_bss_wep_key), NULL) != NULL) {
                    exec_cmd2("echo \"config.wds_key%d.%d=%s\" > %s", kidx, rule, wl_bss_wep_key, PROCWLA);
                }
                else {
                    MTC_LOGGER(this, LOG_ERR, "%s: Can't get %s for wl_wds", __func__, wl_bss_wep_keyname);
                }
            }
            else {
                // FIXME: I don't know what's purpose for this script, skip wait implement
#if  0
config_get wl_bss_wpa_psk1 wl_bss_wpa_psk1
MSG=`wpa_passphrase $ssid $wl_bss_wpa_psk1`
PSK=`echo $MSG | awk -F" " '{ printf $4 }'`
eval $PSK
echo "config.wds_key0=${psk}" > $PROC
#endif
                MTC_LOGGER(this, LOG_INFO, "%s: wait for implement here(line:%d)", __func__, __LINE__);
            }
        }
    }
}

//macf rule $wl_macf1='en=1&smac=a0:11:22:33:44:51&desc=My iPhone'
static void config_hostapd_acl(int wl_macf_mode)
{
    char wl_macf[MSBUF_LEN] = { 0 };
    char *argv[10];
    char *en, *smac;
    int num;
    int i;

    unlink(HOSTAPD_ACLFILE);

    if ((wl_macf_mode == 1) || (wl_macf_mode == 2)) {
        for (i=1;;i++) {
            if (cdb_get_array_str("$wl_macf", i, wl_macf, sizeof(wl_macf), NULL) == NULL) {
                break;
            }
            else if (strlen(wl_macf) == 0) {
                break;
            }
            else {
                MTC_LOGGER(this, LOG_INFO, "%s: wl_macf%d is [%s]", __func__, i, wl_macf);

                num = str2argv(wl_macf, argv, '&');
                if((num < 3)               || !str_arg(argv, "en=") || 
                   !str_arg(argv, "smac=") || !str_arg(argv, "desc=")) { 
                    MTC_LOGGER(this, LOG_INFO, "%s: skip, argv is not complete", __func__);
                    continue;
                }
                en   = str_arg(argv, "en=");
                smac = str_arg(argv, "smac=");

                MTC_LOGGER(this, LOG_INFO, "%s: en is [%s]", __func__, en);
                MTC_LOGGER(this, LOG_INFO, "%s: smac is [%s]", __func__, smac);

                if (!strcmp(en, "1")) {
                    exec_cmd2("echo \"%s\" >> %s", smac, HOSTAPD_ACLFILE);
                }
            }
        }
        exec_cmd2("echo -e \"\n\" >> %s", HOSTAPD_ACLFILE);
    }
}

static void config_general(void)
{
    int wl_bb_mode, wl_2choff, wl_sgi, wl_preamble;
    int wl_bss_int;

    // AP/BSS isolated
    wl_bss_int = cdb_get_int("$wl_ap_isolated", 0);
    exec_cmd2("echo \"config.wl_ap_isolated=%d\" > %s", wl_bss_int, PROCWLA);
    wl_bss_int = cdb_get_int("$wl_bss_isolated", 0);
    exec_cmd2("echo \"config.wl_bss_isolated=%d\" > %s", wl_bss_int, PROCWLA);

    // AMPDU
    wl_bss_int = cdb_get_int("$wl_no_ampdu", 0);
    exec_cmd2("echo \"config.no_ampdu=%d\" > %s", wl_bss_int, PROCWLA);
    wl_bss_int = cdb_get_int("$wl_ampdu_tx_mask", 0);
    exec_cmd2("echo \"config.ampdu_tx_mask=%d\" > %s", wl_bss_int, PROCWLA);
    wl_bss_int = cdb_get_int("$wl_recovery", 0);
    exec_cmd2("echo \"config.recovery=%d\" > %s", wl_bss_int, PROCWLA);

    // special config
    // cwnd and aifs
    wl_bss_int = cdb_get_int("$wl_cw_aifs", 0);
    exec_cmd2("echo \"config.wl_cw_aifs=%d\" > %s", wl_bss_int, PROCWLA);

    // wds with ampdu
    wl_bss_int = cdb_get_int("$wl_wds_ampdu", 0);
    exec_cmd2("echo \"config.wl_wds_ampdu=%d\" > %s", wl_bss_int, PROCWLA);

    // Fix rate
    wl_bss_int = cdb_get_int("$wl_tx_rate", 0);
    exec_cmd2("echo \"config.fix_txrate=%d\" > %s", wl_bss_int, PROCWLA);

    // Fix rate flags
    wl_bss_int = 0;
    wl_bb_mode = cdb_get_int("$wl_bb_mode", 0);
    wl_2choff = cdb_get_int("$wl_2choff", 0);
    wl_sgi = cdb_get_int("$wl_sgi", 0);
    wl_preamble = cdb_get_int("$wl_preamble", 0);
    if (wl_bb_mode >= BB_MODE_N) {
        if (wl_2choff > 0) {
            wl_bss_int |= (1 << 2);
        }
        if (wl_sgi == 1) {
            wl_bss_int |= (1 << 0);
            if (wl_2choff > 0) {
                wl_bss_int |= (1 << 1);
            }
        }
    }
    if (wl_preamble == 0) {
        wl_bss_int |= (1 << 4);
    }
    exec_cmd2("echo \"config.rate_flags=%d\" > %s", wl_bss_int, PROCWLA);

    if (mtc->rule.HOSTAPD == 1) {
        wl_bss_int = cdb_get_int("$wl_forward_mode", 0);
        exec_cmd2("echo \"config.wl_forward_mode=%d\" > %s", wl_bss_int, PROCWLA);

        wl_bss_int = cdb_get_int("$wl_tx_pwr", 0);
        exec_cmd2("echo \"config.wl_tx_pwr=%d\" > %s", wl_bss_int, PROCWLA);

        if (cdb_get_int("$wl_protection", 0) == 1) {
            exec_cmd2("echo \"config.bg_protection=1\" > %s", PROCWLA);
        }
        else {
            exec_cmd2("echo \"config.bg_protection=0\" > %s", PROCWLA);
        }

        exec_cmd2("echo \"config.wds_mode=0\" > %s", PROCWLA);

        wl_bss_int = cdb_get_int("$wl_wds_mode", 0);
        if ((wl_bss_int == 1) || (wl_bss_int == 2)) {
            exec_cmd2("echo \"config.wds_mode=%d\" > %s", wl_bss_int, PROCWLA);
            exec_cmd2("echo \"config.wds_peer_addr.0=0\" > %s", PROCWLA);
            exec_cmd2("echo \"config.wds_peer_addr.1=0\" > %s", PROCWLA);
            exec_cmd2("echo \"config.wds_peer_addr.2=0\" > %s", PROCWLA);
            exec_cmd2("echo \"config.wds_peer_addr.3=0\" > %s", PROCWLA);

            config_wds();
        }
        else {
            exec_cmd2("echo \"config.wds_mode=0\" > %s", PROCWLA);
        }

        // Power Saving
        wl_bss_int = cdb_get_int("$wl_ps_mode", 0);
        exec_cmd2("echo \"config.pw_save=%d\" > %s", wl_bss_int, PROCWLA);
        if (wl_bss_int == 2) {
            exec_cmd2("echo \"config.uapsd=1\" > %s", PROCWLA);
        }
        else {
            exec_cmd2("echo \"config.uapsd=0\" > %s", PROCWLA);
        }
    }
    if (mtc->rule.WPASUP == 1) {
        wl_bss_int = cdb_get_int("$wl_forward_mode", 0);
        exec_cmd2("echo \"config.wl_forward_mode=%d\" > %s", wl_bss_int, PROCWLA);
        
        if ((cdb_get_int("$wl_bss_role2", 0) & 256) > 0) {
            exec_cmd2("echo \"config.sta_mat=1\" > %s", PROCWLA);
        }
        else {
            exec_cmd2("echo \"config.sta_mat=0\" > %s", PROCWLA);
        }
    }
}

//wl_aplist_info=bssid=xxx&sec=xxx&cipher=&last=&ssid=
//wl_aplist_pass=xxx
static void config_ap_list(FILE *fp)
{
    char wl_aplist_info[MSBUF_LEN] = { 0 };
    char wl_aplist_pass[MSBUF_LEN] = { 0 };
    char *argv[10];
    char *bssid, *sec, *cipher, *last, *ssid;
    char *pass = wl_aplist_pass;
    int num;
    int i;

    for (i=1;i<11;i++) {
        if (cdb_get_array_str("$wl_aplist_info", i, wl_aplist_info, sizeof(wl_aplist_info), NULL) == NULL) {
            break;
        }
        else if (strlen(wl_aplist_info) == 0) {
            break;
        }
        else {
            cdb_get_array_str("$wl_aplist_pass", i, wl_aplist_pass, sizeof(wl_aplist_pass), "");
            MTC_LOGGER(this, LOG_INFO, "%s: wl_aplist_info%d is [%s]", __func__, i, wl_aplist_info);
            MTC_LOGGER(this, LOG_INFO, "%s: wl_aplist_pass%d is [%s]", __func__, i, wl_aplist_pass);

            num = str2argv(wl_aplist_info, argv, '&');
            if((num < 5)               || !str_arg(argv, "bssid=")  || 
               !str_arg(argv, "sec=")  || !str_arg(argv, "cipher=") || 
               !str_arg(argv, "last=") || !str_arg(argv, "ssid=")) {
                MTC_LOGGER(this, LOG_INFO, "%s: skip, argv is not complete", __func__);
                continue;
            }
            bssid  = str_arg(argv, "bssid=");
            sec    = str_arg(argv, "sec=");
            cipher = str_arg(argv, "cipher=");
            last   = str_arg(argv, "last=");
            ssid   = str_arg(argv, "ssid=");

            MTC_LOGGER(this, LOG_INFO, "%s: bssid  is [%s]", __func__, bssid);
            MTC_LOGGER(this, LOG_INFO, "%s: sec    is [%s]", __func__, sec);
            MTC_LOGGER(this, LOG_INFO, "%s: cipher is [%s]", __func__, cipher);
            MTC_LOGGER(this, LOG_INFO, "%s: last   is [%s]", __func__, last);
            MTC_LOGGER(this, LOG_INFO, "%s: ssid   is [%s]", __func__, ssid);

            fprintf(fp, "network={ \n");
            fprintf(fp, "ssid=\"%s\"\n", ssid);
            fprintf(fp, "bssid=%s\n", bssid);
            if (!strcmp(sec, "none")) {
                fprintf(fp, "key_mgmt=NONE\n");
            }
            else if (!strcmp(sec, "wep")) {
                fprintf(fp, "key_mgmt=NONE\n");
                fprintf(fp, "wep_key0=%s\n", pass);
                fprintf(fp, "wep_key1=%s\n", pass);
                fprintf(fp, "wep_key2=%s\n", pass);
                fprintf(fp, "wep_key3=%s\n", pass);
                fprintf(fp, "wep_tx_keyidx=0\n");
            }
            else {
                fprintf(fp, "key_mgmt=WPA-PSK\n");
                fprintf(fp, "pairwise=%s\n", cipher);
                fprintf(fp, "psk=\"%s\"\n", pass);
            }
            if (!strcmp(last, "1")) {
                fprintf(fp, "priority=1\n");
            }
            fprintf(fp, "}\n");
        }
    }
}

static void config_bss_cipher(FILE *fp, int bss, WLRole role)
{
    char wl_bss_bssid2[MBUF_LEN] = { 0 };
    char wl_bss_bssid3[MBUF_LEN] = { 0 };
    int wl_bss_sec_type = cdb_get_array_int("$wl_bss_sec_type", bss, 0);

    MTC_LOGGER(this, LOG_INFO, "%s: config bss[%d] role[%d]", __func__, bss, role);
    MTC_LOGGER(this, LOG_INFO, "%s: wl_bss_sec_type%d is [%d]", __func__, bss, wl_bss_sec_type);

    if (bss != 1) {
        if (role == wlRole_AP) {
            if (cdb_get_array_str("$wl_bss_bssid", 3, wl_bss_bssid3, sizeof(wl_bss_bssid3), NULL) == NULL) {
                MTC_LOGGER(this, LOG_ERR, "%s: wl_bss_bssid3 is unknown", __func__);
            }
            else {
                MTC_LOGGER(this, LOG_INFO, "%s: wl_bss_bssid3 is [%s]", __func__, wl_bss_bssid3);
                fprintf(fp, "bssid=%s\n", wl_bss_bssid3);
            }
        }
        else if (role == wlRole_STA) {
            if (cdb_get_array_str("$wl_bss_bssid", 2, wl_bss_bssid2, sizeof(wl_bss_bssid2), NULL) == NULL) {
                MTC_LOGGER(this, LOG_ERR, "%s: wl_bss_bssid2 is unknown", __func__);
            }
            else {
                MTC_LOGGER(this, LOG_INFO, "%s: wl_bss_bssid2 is [%s]", __func__, wl_bss_bssid2);
                fprintf(fp, "bssid=%s\n", wl_bss_bssid2);
            }
        }
    }
    if (wl_bss_sec_type != 0) {
        if (wl_bss_sec_type & AUTH_CAP_WPS) {
            if (role == wlRole_AP) {
                char wl_wps_def_pin[SSBUF_LEN] = { 0 };
                int wl_bss_wps_state = cdb_get_array_int("$wl_bss_wps_state", bss, 0);

                fprintf(fp, "wps_state=%d\n", wl_bss_wps_state);
                fprintf(fp, "eap_server=1\n");
//                fprintf(fp, "ap_setup_locked=1\n");
                fprintf(fp, "device_name=Cheetah AP\n");
                fprintf(fp, "manufacturer=Montage\n");
                fprintf(fp, "model_name=Cheetah\n");
                fprintf(fp, "model_number=3280\n");
                fprintf(fp, "serial_number=12345\n");
                fprintf(fp, "device_type=6-0050F204-1\n");
                fprintf(fp, "os_version=01020300\n");
                fprintf(fp, "config_methods=label virtual_display virtual_push_button keypad\n");
                fprintf(fp, "upnp_iface=br0\n");

                cdb_get_str("$wl_wps_def_pin", wl_wps_def_pin, sizeof(wl_wps_def_pin), "");
                if (strlen(wl_wps_def_pin) == 0) {
                    if (exec_cmd3(wl_wps_def_pin, sizeof(wl_wps_def_pin), 
                            "cat /proc/bootvars | grep pin= | cut -b 5-") == 0) {
                        cdb_set("$wl_wps_def_pin", wl_wps_def_pin);
                    }
                }
                MTC_LOGGER(this, LOG_INFO, "%s: wl_bss_wps_state%d is [%d]", __func__, bss, wl_bss_wps_state);
                MTC_LOGGER(this, LOG_INFO, "%s: wl_wps_def_pin is [%s]", __func__, wl_wps_def_pin);
                fprintf(fp, "ap_pin=%s\n", wl_wps_def_pin);
            }
            else {
                if ((wl_bss_sec_type & ~AUTH_CAP_WPS) <= AUTH_CAP_OPEN) {
                    fprintf(fp, "key_mgmt=NONE\n");
                }
            }
        }

        if (wl_bss_sec_type & AUTH_CAP_WEP) {
            char wl_bss_wep_key[MSBUF_LEN] = { 0 };
            char wl_bss_wep_keyname[USBUF_LEN];
            int wl_bss_wep_index = cdb_get_array_int("$wl_bss_wep_index", bss, 0);
            int len, i;
            if (role == wlRole_AP) {
                fprintf(fp, "wep_default_key=%d\n", wl_bss_wep_index);
            }
            else {
                fprintf(fp, "key_mgmt=NONE\n");
                fprintf(fp, "wep_tx_keyidx=%d\n", wl_bss_wep_index);
            }
            MTC_LOGGER(this, LOG_INFO, "%s: WEP:", __func__);
            MTC_LOGGER(this, LOG_INFO, "%s: wl_bss_wep_index%d is [%d]", __func__, bss, wl_bss_wep_index);
            for (i=1;i<5;i++) {
                sprintf(wl_bss_wep_keyname, "wl_bss_wep_%dkey", i);
                if (cdb_get_array_str(wl_bss_wep_keyname, 
                    bss, wl_bss_wep_key, sizeof(wl_bss_wep_key), NULL) != NULL) {
                    len = strlen(wl_bss_wep_key);
                    if ((len == 5) || (len == 13)) {
                        fprintf(fp, "wep_key%d=\"%s\"\n", (i-1), wl_bss_wep_key);
                    }
                    else {
                        fprintf(fp, "wep_key%d=%s\n", (i-1), wl_bss_wep_key);
                    }
                    MTC_LOGGER(this, LOG_INFO, "%s: %s is [%s]", __func__, wl_bss_wep_keyname, wl_bss_wep_key);
                }
            }
            if ((role == wlRole_AP) && (wl_bss_sec_type & AUTH_CAP_SHARED)) {
                fprintf(fp, "auth_algs=2\n");
            }
        }

        if (wl_bss_sec_type & (AUTH_CAP_WPA | AUTH_CAP_WPA2)) {
            char wl_bss_wpa_psk[MSBUF_LEN] = { 0 };
            int wl_bss_wpa_rekey = cdb_get_array_int("$wl_bss_wpa_rekey", bss, 0);
            int wl_bss_cipher = cdb_get_array_int("$wl_bss_cipher", bss, 0);
            int wl_bss_key_mgt = cdb_get_array_int("$wl_bss_key_mgt", bss, 0);
            cdb_get_array_str("$wl_bss_wpa_psk", bss, wl_bss_wpa_psk, sizeof(wl_bss_wpa_psk), "");
            if (wl_bss_key_mgt > 0) {
                fprintf(fp, "ieee80211x=1\n");
                fprintf(fp, "eapol_key_index_workaround=0\n");
                fprintf(fp, "wpa_key_mgmt=WPA-EAP\n");
            }
            else {
                if (role == wlRole_AP) {
                    fprintf(fp, "wpa_key_mgmt=WPA-PSK\n");
                }
                else {
                    fprintf(fp, "key_mgmt=WPA-PSK\n");
                }
            }
            if (role == wlRole_AP) {
                if ((wl_bss_sec_type & (AUTH_CAP_WPA | AUTH_CAP_WPA2)) == (AUTH_CAP_WPA | AUTH_CAP_WPA2)) {
                    fprintf(fp, "wpa=3\n");
                }
                else if (wl_bss_sec_type & AUTH_CAP_WPA2) {
                    fprintf(fp, "wpa=2\n");
                }
                else if (wl_bss_sec_type & AUTH_CAP_WPA) {
                    fprintf(fp, "wpa=1\n");
                }
                fprintf(fp, "wpa_pairwise=");
                if (wl_bss_cipher & AUTH_CAP_CIPHER_TKIP) {
                    fprintf(fp, "TKIP ");
                }
                if (wl_bss_cipher & AUTH_CAP_CIPHER_CCMP) {
                    fprintf(fp, "CCMP ");
                }
                fprintf(fp, "\n");
            }
            else {
                fprintf(fp, "pairwise=");
                if (wl_bss_cipher & AUTH_CAP_CIPHER_TKIP) {
                    fprintf(fp, "TKIP ");
                }
                if (wl_bss_cipher & AUTH_CAP_CIPHER_CCMP) {
                    fprintf(fp, "CCMP ");
                }
                fprintf(fp, "\n");
            }
            if (wl_bss_key_mgt > 0) {
                char wl_bss_radius_svr[MSBUF_LEN] = { 0 };
                char wl_bss_radius_svr_key[MSBUF_LEN] = { 0 };
                int wl_bss_radius_svr_port = cdb_get_array_int("$wl_bss_radius_svr_port", bss, 0);
                cdb_get_array_str("$wl_bss_radius_svr", bss, wl_bss_radius_svr, sizeof(wl_bss_radius_svr), "");
                cdb_get_array_str("$wl_bss_radius_svr_key", bss, wl_bss_radius_svr_key, sizeof(wl_bss_radius_svr_key), "");
                fprintf(fp, "own_ip_addr=127.0.0.1\n");
                fprintf(fp, "auth_server_addr=%s\n", wl_bss_radius_svr);
                fprintf(fp, "auth_server_port=%d\n", wl_bss_radius_svr_port);
                fprintf(fp, "auth_server_shared_secret=%s\n", wl_bss_radius_svr_key);
            }
            else {
                if (role == wlRole_AP) {
                    if (strlen(wl_bss_wpa_psk) == 64) {
                        fprintf(fp, "wpa_psk=%s\n", wl_bss_wpa_psk);
                    }
                    else {
                        fprintf(fp, "wpa_passphrase=%s\n", wl_bss_wpa_psk);
                    }
                }
                else {
                    if (strlen(wl_bss_wpa_psk) == 64) {
                        fprintf(fp, "psk=%s\n", wl_bss_wpa_psk);
                    }
                    else {
                        fprintf(fp, "psk=\"%s\"\n", wl_bss_wpa_psk);
                    }
                }
            }
            if (role == wlRole_AP) {
                fprintf(fp, "wpa_group_rekey=%d\n", wl_bss_wpa_rekey);
            }
            MTC_LOGGER(this, LOG_INFO, "%s: %s:", __func__, (wl_bss_sec_type & AUTH_CAP_WPA) ? "WPA" : "WPA2");
            MTC_LOGGER(this, LOG_INFO, "%s: wl_bss_cipher%d is [%d]", __func__, bss, wl_bss_cipher);
            MTC_LOGGER(this, LOG_INFO, "%s: wl_bss_key_mgt%d is [%d]", __func__, bss, wl_bss_key_mgt);
            MTC_LOGGER(this, LOG_INFO, "%s: wl_bss_wpa_psk%d is [%s]", __func__, bss, wl_bss_wpa_psk);
            MTC_LOGGER(this, LOG_INFO, "%s: wpa_group_rekey%d is [%d]", __func__, bss, wl_bss_wpa_rekey);
        }

        if (wl_bss_sec_type & AUTH_CAP_WAPI) {
            char wl_bss_wpa_psk[MSBUF_LEN] = { 0 };
            int wl_bss_wpa_rekey = cdb_get_array_int("$wl_bss_wpa_rekey", bss, 0);
            cdb_get_array_str("$wl_bss_wpa_psk", bss, wl_bss_wpa_psk, sizeof(wl_bss_wpa_psk), "");
            fprintf(fp, "wapi=1\n");
            if (role == wlRole_AP) {
                fprintf(fp, "wpa_passphrase=%s\n", wl_bss_wpa_psk);
                fprintf(fp, "wpa_group_rekey=%d\n", wl_bss_wpa_rekey);
            }
            else {
                fprintf(fp, "psk=\"%s\"\n", wl_bss_wpa_psk);
                // Unneccessary apply ptk/group rekey parameter to STA, 
                // rekey mechanism created by AP.
            }
            MTC_LOGGER(this, LOG_INFO, "%s: WAPI:", __func__);
            MTC_LOGGER(this, LOG_INFO, "%s: wl_bss_wpa_psk%d is [%s]", __func__, bss, wl_bss_wpa_psk);
            MTC_LOGGER(this, LOG_INFO, "%s: wpa_group_rekey%d is [%d]", __func__, bss, wl_bss_wpa_rekey);
        }
    }
    else {
        if (role == wlRole_STA) {
            fprintf(fp, "key_mgmt=NONE\n");
        }
    }
}

static int check_hostapd_terminated(void)
{
    if( f_exists(HOSTAPD_PIDFILE) ) {
        while (1) {
            int pid = read_pid_file(HOSTAPD_PIDFILE);
            if ((pid == UNKNOWN) || (exec_cmd2("kill -s 9 %d 2>/dev/null", pid) != 0))
                break;
            sleep(1);
            MTC_LOGGER(this, LOG_INFO, "%s: hostapd pid [%d]", __func__, pid);
        }
    }
    return 0;
}

static int mac80211_hostapd_setup_base(void)
{
    char wl_bss_ssid1[MSBUF_LEN] = { 0 };
    char wl_region[USBUF_LEN] = { 0 };
    char hostapd_acl[MSBUF_LEN] = { 0 };
    char basic_rates[USBUF_LEN] = { 0 };
    char base_cfg[LSBUF_LEN] = { 0 };
    char ht_capab[SSBUF_LEN] = { 0 };
    char bss1_cfg[SSBUF_LEN] = { 0 };
#ifdef HAVE_MULTIPLE_BSSID
    char bss2_cfg[SSBUF_LEN] = { 0 };
#endif
    char hw_mode[USBUF_LEN] = { 0 };
    int wl_beacon_listen = cdb_get_int("$wl_beacon_listen", -1);
    int wl_macf_mode = cdb_get_int("$wl_macf_mode", 0);
    int wl_preamble = 0;
    int wl_bb_mode = 0;
    int wl_channel = 0;
    int wl_2choff = 0;
    int wl_sgi = 0;
    int wl_tbtt = 0;
    int wl_dtim = 0;
    int wl_apsd = 0;
    int wl_frag = 0;
    int wl_rts = 0;

    unsigned char val[6];
    char buf[MSBUF_LEN] = { 0 };
    char *ptr;
    FILE *fp;
    int num;
    int i;

    cdb_get_str("$wl_region", wl_region, sizeof(wl_region), "US");

    cdb_get_array_str("$wl_bss_ssid", 1, wl_bss_ssid1, sizeof(wl_bss_ssid1), "");

    if (exec_cmd3(buf, sizeof(buf), "echo \"%s\" | sed -e 's/_%%[1-6]x$//'", wl_bss_ssid1) == 0) {
        if (strcmp(buf, wl_bss_ssid1)) {
            my_mac2val(val, mtc->boot.MAC0);
            ptr = wl_bss_ssid1;
            while (*ptr) {
                if (isdigit(*ptr))
                    break;
                ptr++;
            }
            num = strtoul(ptr, NULL, 0);
            ptr = wl_bss_ssid1;
            ptr += sprintf(ptr, "%s_", buf);
            if (num > sizeof(val)) {
                num = sizeof(val);
            }
            if (num > 0) {
                for (i=num;i>0;i--) {
                    ptr += sprintf(ptr, "%2.2x", val[sizeof(val)-i]);
                }
            }
            cdb_set("$wl_bss_ssid1", wl_bss_ssid1);
            MTC_LOGGER(this, LOG_INFO, "%s: new wl_bss_ssid1 is [%s]", __func__, wl_bss_ssid1);
        }
    }

    // basic_rates
    num = cdb_get_int("$wl_basic_rates", 0);
    if ((num == 0) || (num == 127)) {
        sprintf(basic_rates, "10 20 55 60 110 120 240");
    }
    else if (num == 3) {
        sprintf(basic_rates, "10 20");
    }
    else if (num == 16) {
        sprintf(basic_rates, "10 20 55 110");
    }

    // wl_apsd
    if ((wl_apsd = cdb_get_int("$wl_ps_mode", 0)) == 2) {
        wl_apsd = 1;
    }
    else {
        wl_apsd = 0;
    }

    // wl_preamble
    if ((wl_preamble = cdb_get_int("$wl_preamble", 1)) == 0) {
        wl_preamble = 1;
    }
    else {
        wl_preamble = 0;
    }

    // wl_tbtt
    if ((wl_tbtt = cdb_get_int("$wl_tbtt", 100)) < 15) {
        wl_tbtt = 15;
    }
    else if (wl_tbtt > 65535) {
        wl_tbtt = 65535;
    }

    // wl_dtim
    if ((wl_dtim = cdb_get_int("$wl_dtim", 2)) < 1) {
        wl_dtim = 1;
    }
    else if (wl_dtim > 255) {
        wl_dtim = 255;
    }

    // wl_rts
    if ((wl_rts = cdb_get_int("$wl_rts", 2347)) < 256) {
        wl_rts = 256;
    }
    else if (wl_rts > 2347) {
        wl_rts = 2347;
    }

    // wl_frag
    if ((wl_frag = cdb_get_int("$wl_frag", 2346)) < 256) {
        wl_frag = 256;
    }
    else if (wl_frag > 2346) {
        wl_frag = 2346;
    }

    // base_cfg, ht_capab
    if ((wl_bb_mode = cdb_get_int("$wl_bb_mode", BB_MODE_G)) == BB_MODE_B) {
        sprintf(hw_mode, "b");
    }
    else {
        sprintf(hw_mode, "g");
    }
    ptr = base_cfg;
    if (wl_bb_mode >= BB_MODE_N) {
        wl_channel = cdb_get_int("$wl_channel", 0);
        wl_2choff = cdb_get_int("$wl_2choff", 0);
        wl_sgi = cdb_get_int("$wl_sgi", 0);

        ptr = ht_capab;
        switch (wl_2choff) {
            case 0:
                if (wl_sgi == 1) {
                    ptr += sprintf(ptr, "[SHORT-GI-20]");
                }
                break;
            case 1:
                ptr += sprintf(ptr, "[HT40+]");
                if (wl_sgi == 1) {
                    ptr += sprintf(ptr, "[SHORT-GI-20][SHORT-GI-40]");
                }
                break;
            case 2:
                ptr += sprintf(ptr, "[HT40-]");
                if (wl_sgi == 1) {
                    ptr += sprintf(ptr, "[SHORT-GI-20][SHORT-GI-40]");
                }
                break;
            case 3:
                if (wl_channel == 0) {
                    ptr += sprintf(ptr, "[HT40+]");
                }
                else {
                    ptr += sprintf(ptr, "[HT40-AUTO]");
                }
                if (wl_sgi == 1) {
                    ptr += sprintf(ptr, "[SHORT-GI-20][SHORT-GI-40]");
                }
                break;
            default:
                break;
        }

        if (cdb_get_int("$wl_ht_mode", 0) == 1) {
            ptr += sprintf(ptr, "[GF]");
        }
        ptr += sprintf(ptr, "[SMPS-STATIC]");


        ptr = base_cfg;
        ptr += sprintf(ptr, "ieee80211n=1\n");
        ptr += sprintf(ptr, "ht_capab=%s\n", ht_capab);
        if (cdb_get_int("$wl_2040_coex", 0) == 1) {
            ptr += sprintf(ptr, "noscan=0\n");
            ptr += sprintf(ptr, "2040_coex_en=1\n");
        }
        else if (wl_beacon_listen != 1) {
            ptr += sprintf(ptr, "noscan=1\n");
        }
    }
    // ptr here, it should be point to base_cfg
    if (wl_beacon_listen != -1) {
        ptr += sprintf(ptr, "obss_survey_en=%d\n", wl_beacon_listen);
    }

    // bss1_cfg, wds
    ptr = bss1_cfg;
    if (cdb_get_int("$wl_wds_mode", 0) == 1) {
        ptr += sprintf(ptr, "ssid=___wds_bridge___\n");
        ptr += sprintf(ptr, "ignore_broadcast_ssid=1\n");
    }
    else {
        ptr += sprintf(ptr, "ssid=%s\n", wl_bss_ssid1);
        if (cdb_get_int("$wl_bss_ssid_hidden1", 0) == 1) {
            ptr += sprintf(ptr, "ignore_broadcast_ssid=1\n");
        }
        else
            ptr += sprintf(ptr, "ignore_broadcast_ssid=0\n");
    }

#ifdef HAVE_MULTIPLE_BSSID
    // bss2_cfg, multiple bssid
    ptr = bss2_cfg;
    if (cdb_get_int("$wl_bss_enable3", 0) == 1) {
        if (cdb_get_int("$wl_bss_role3", 0) & wlRole_AP) {
            cdb_get_array_str("$wl_bss_ssid", 3, ssid, sizeof(ssid), "");
            ptr += sprintf(ptr, "bss=wlan1\n");
            ptr += sprintf(ptr, "ssid=%s\n", ssid);
            if (cdb_get_int("$wl_bss_ssid_hidden3", 0) == 1) {
                ptr += sprintf(ptr, "ignore_broadcast_ssid=1\n");
            }
#warning "not implement yet"
            config_bss_cipher(0, 3, wlRole_AP); 
        }
    }
#endif

    // hostapd_acl
    ptr = hostapd_acl;
    if (wl_macf_mode == 0) {
        ptr += sprintf(ptr, "macaddr_acl=0\n");
    }
    else if (wl_macf_mode == 1) {
        ptr += sprintf(ptr, "macaddr_acl=0\n");
        ptr += sprintf(ptr, "deny_mac_file=%s\n", HOSTAPD_ACLFILE);
    }
    else if (wl_macf_mode == 2) {
        ptr += sprintf(ptr, "macaddr_acl=1\n");
        ptr += sprintf(ptr, "accept_mac_file=%s\n", HOSTAPD_ACLFILE);
    }
    config_hostapd_acl(wl_macf_mode);

    // really 
    fp = fopen(HOSTAPD_CFGFILE, "w");
    if (fp) {
        fprintf(fp, "interface=%s\n", mtc->rule.AP);
        fprintf(fp, "driver=nl80211\n");
        fprintf(fp, "fragm_threshold=%d\n", wl_frag);
        fprintf(fp, "rts_threshold=%d\n", wl_rts);
        fprintf(fp, "beacon_int=%d\n", wl_tbtt);
        fprintf(fp, "dtim_period=%d\n", wl_dtim);
        fprintf(fp, "preamble=%d\n", wl_preamble);
        fprintf(fp, "uapsd_advertisement_enabled=%d\n", wl_apsd);
        fprintf(fp, "channel=%d\n", wl_channel);
        fprintf(fp, "hw_mode=%s\n", hw_mode);
        fprintf(fp, "basic_rates=%s\n", basic_rates);
        fprintf(fp, "%s\n", hostapd_acl);
        fprintf(fp, "wmm_enabled=%d\n", cdb_get_int("$wl_bss_wmm_enable1", 0));
        fprintf(fp, "wmm_ac_bk_cwmin=4\n");
        fprintf(fp, "wmm_ac_bk_cwmax=10\n");
        fprintf(fp, "wmm_ac_bk_aifs=7\n");
        fprintf(fp, "wmm_ac_bk_txop_limit=0\n");
        fprintf(fp, "wmm_ac_bk_acm=0\n");
        fprintf(fp, "wmm_ac_be_aifs=3\n");
        fprintf(fp, "wmm_ac_be_cwmin=4\n");
        fprintf(fp, "wmm_ac_be_cwmax=10\n");
        fprintf(fp, "wmm_ac_be_txop_limit=0\n");
        fprintf(fp, "wmm_ac_be_acm=0\n");
        fprintf(fp, "wmm_ac_vi_aifs=2\n");
        fprintf(fp, "wmm_ac_vi_cwmin=3\n");
        fprintf(fp, "wmm_ac_vi_cwmax=4\n");
        fprintf(fp, "wmm_ac_vi_txop_limit=94\n");
        fprintf(fp, "wmm_ac_vi_acm=0\n");
        fprintf(fp, "wmm_ac_vo_aifs=2\n");
        fprintf(fp, "wmm_ac_vo_cwmin=2\n");
        fprintf(fp, "wmm_ac_vo_cwmax=3\n");
        fprintf(fp, "wmm_ac_vo_txop_limit=47\n");
        fprintf(fp, "wmm_ac_vo_acm=0\n");
        fprintf(fp, "tx_queue_data3_aifs=7\n");
        fprintf(fp, "tx_queue_data3_cwmin=15\n");
        fprintf(fp, "tx_queue_data3_cwmax=1023\n");
        fprintf(fp, "tx_queue_data3_burst=0\n");
        fprintf(fp, "tx_queue_data2_aifs=3\n");
        fprintf(fp, "tx_queue_data2_cwmin=15\n");
        fprintf(fp, "tx_queue_data2_cwmax=63\n");
        fprintf(fp, "tx_queue_data2_burst=0\n");
        fprintf(fp, "tx_queue_data1_aifs=1\n");
        fprintf(fp, "tx_queue_data1_cwmin=7\n");
        fprintf(fp, "tx_queue_data1_cwmax=15\n");
        fprintf(fp, "tx_queue_data1_burst=3.0\n");
        fprintf(fp, "tx_queue_data0_aifs=1\n");
        fprintf(fp, "tx_queue_data0_cwmin=3\n");
        fprintf(fp, "tx_queue_data0_cwmax=7\n");
        fprintf(fp, "tx_queue_data0_burst=1.5\n");
        fprintf(fp, "country_code=%s\n", wl_region);
        fprintf(fp, "ieee80211d=1\n");
        fprintf(fp, "%s\n", base_cfg);
        fprintf(fp, "%s\n", bss1_cfg);
        fprintf(fp, "ctrl_interface=/var/run/hostapd\n");
        config_bss_cipher(fp, 1, wlRole_AP);
#ifdef HAVE_MULTIPLE_BSSID
        fprintf(fp, "%s\n", bss2_cfg);
        config_bss_cipher(fp, 3, wlRole_AP);
#endif
#ifdef CONFIG_WLAN_AP_INACTIVITY_TIME_30
        fprintf(fp, "ap_max_inactivity=30\n");
#endif
        fclose(fp);
    }
    else {
        MTC_LOGGER(this, LOG_ERR, "%s: open [%s] fail", __func__, HOSTAPD_CFGFILE);
    }

    return 0;
}

static int mac80211_wpasup_setup_base(void)
{
    char wl_bss_ssid2[MSBUF_LEN] = { 0 };
    char wl_region[USBUF_LEN] = { 0 };
    u8 scan_interval = (mtc->rule.OMODE == opMode_wi) ? 40 : 0;
    FILE *fp;

    cdb_get_str("$wl_region", wl_region, sizeof(wl_region), "US");
    MTC_LOGGER(this, LOG_INFO, "%s: wl_region is [%s]", __func__, wl_region);

    if (cdb_get_int("$wl_aplist_en", 0) == 1) {
        MTC_LOGGER(this, LOG_INFO, "%s: wl_aplist_en is enabled", __func__);

        exec_cmd("aplist");
        fp = fopen(WPASUP_CFGFILE, "w");
        if (fp) {
            fprintf(fp, "ctrl_interface=/var/run/wpa_supplicant\n");
            config_ap_list(fp);
            fprintf(fp, "scan_interval=%u\n", scan_interval);
            fprintf(fp, "country=%s\n", wl_region);
            fclose(fp);
        }
    }
    else {
        MTC_LOGGER(this, LOG_INFO, "%s: wl_aplist_en is disabled", __func__);

        if (cdb_get_array_str("$wl_bss_ssid", 2, wl_bss_ssid2, sizeof(wl_bss_ssid2), NULL) == NULL) {
            MTC_LOGGER(this, LOG_ERR, "%s: wl_bss_ssid2 is unknown", __func__);
        }
        else {
            MTC_LOGGER(this, LOG_INFO, "%s: wl_bss_ssid2 is [%s]", __func__, wl_bss_ssid2);
        }

        fp = fopen(WPASUP_CFGFILE, "w");
        if (fp) {
            fprintf(fp, "ctrl_interface=/var/run/wpa_supplicant\n");
            fprintf(fp, "network={\n");
            fprintf(fp, "ssid=\"%s\"\n", wl_bss_ssid2);
            config_bss_cipher(fp, 2, wlRole_STA);
            fprintf(fp, "}\n");
            fprintf(fp, "scan_interval=%u\n", scan_interval);
            fprintf(fp, "country=%s\n", wl_region);
            fclose(fp);
        }
        else {
            MTC_LOGGER(this, LOG_ERR, "%s: open [%s] fail", __func__, WPASUP_CFGFILE);
        }
    }

    return 0;
}

static int mac80211_p2p_setup_base(void)
{
    FILE *fp;

    fp = fopen(WPASUP_CFGFILE, "w");
    if (fp) {
        fprintf(fp, "ctrl_interface=/var/run/wpa_supplicant\n");
        fprintf(fp, "ap_scan=1\n");
        fprintf(fp, "fast_reauth=1\n");
        fprintf(fp, "device_name=montage_p2p\n");
        fprintf(fp, "device_type=1-0050F204-1\n");
        fprintf(fp, "config_methods= virtual_display virtual_push_button keypad\n");
        fprintf(fp, "p2p_listen_reg_class=81\n");
        fprintf(fp, "p2p_listen_channel=11\n");
        fprintf(fp, "p2p_oper_reg_class=81\n");
        fprintf(fp, "p2p_oper_channel=11\n");
        fclose(fp);
    }
    return 0;
}

static int hostapd_cli_start(void)
{
    char wl_mac_gpio[SSBUF_LEN];
    char mypath[SSBUF_LEN];
    char mac[MBUF_LEN];
    u8 en, gpio;
    u8 val[6];
    FILE *fp;

//  $wl_mac_gpio=en=0&mac=00:32:80:00:11:22&gpio=11
    if (cdb_get_str("$wl_mac_gpio", wl_mac_gpio, sizeof(wl_mac_gpio), NULL) == NULL) {
        MTC_LOGGER(this, LOG_INFO, "%s: No wl_mac_gpio cdb", __func__);
        return 2;
    }
    if (sscanf(wl_mac_gpio, "en=%hhu;mac=%hhx:%hhx:%hhx:%hhx:%hhx:%hhx;gpio=%hhu", 
            &en, &val[0], &val[1], &val[2], &val[3], &val[4], &val[5], &gpio) != 8) {
        MTC_LOGGER(this, LOG_INFO, "%s: No wl_mac_gpio cdb", __func__);
        return 2;
    }
    if ( f_exists(HOSTAPD_CLI_PIDFILE) ) {
        MTC_LOGGER(this, LOG_INFO, "%s: Already run pid, can't start again", __func__);
        return 1;
    }
    // 0 : disable, 1 : active high, 2 : active low
    if (en == 0) {
        return 0;
    }
    sprintf(mac, "%2.2x:%2.2x:%2.2x:%2.2x:%2.2x:%2.2x", val[0], val[1], val[2], val[3], val[4], val[5]);
    if (gpio >= 33) {
        MTC_LOGGER(this, LOG_ERR, "%s: Incorrect setting: \"en=%d;mac=%s;gpio=%d\"", __func__, en, mac, gpio);
        return 2;
    }
    MTC_LOGGER(this, LOG_INFO, "%s: Setup GPIO%d for %s", __func__, gpio, mac);
    sprintf(mypath, "%s/gpio%d", GPIO_PATH, gpio);
    if ( !f_isdir(mypath) ) {
        exec_cmd2("echo %d > /sys/class/gpio/export", gpio);
    }
    exec_cmd2("echo out > /sys/class/gpio/gpio%d/direction", gpio);
    exec_cmd2("echo %d > /sys/class/gpio/gpio%d/value", (en & 1) ? 1 : 0, gpio);

    fp = fopen(HOSTAPD_CLI_ACTIONFILE, "w");
    if (fp) {
        fprintf(fp, "#!/bin/ash\n");
        fprintf(fp, "#\n");
        fprintf(fp, "# cdb: en=%d;mac=%s;gpio=%d\n", en, mac, gpio);
        fprintf(fp, "#\n");
        fprintf(fp, "mode=%d\n", (en & 1) ? 1 : 0);
        fprintf(fp, "[ \"$3\" = \"`echo %s | tr A-Z a-z`\" ] && {\n", mac);
        fprintf(fp, "\t[ \"$2\" = \"AP-STA-CONNECTED\" ] && sw=0\n");
        fprintf(fp, "\t[ \"$2\" = \"AP-STA-DISCONNECTED\" ] && sw=1\n");
        fprintf(fp, "\ton=$(( $sw ^ $mode ))\n");
        fprintf(fp, "\techo $on > /sys/class/gpio/gpio%d/value\n", gpio);
        fprintf(fp, "}\n");
        fclose(fp);
        exec_cmd2("chmod a+x "HOSTAPD_CLI_ACTIONFILE);
        exec_cmd2("start-stop-daemon -S -b -m -p %s -x %s -- %s", HOSTAPD_CLI_PIDFILE, HOSTAPD_CLI_BIN, HOSTAPD_CLI_CMDLINE);
        return 0;
    }
    else {
        MTC_LOGGER(this, LOG_ERR, "%s: Create %s is failed", __func__, HOSTAPD_CLI_ACTIONFILE);
        return 2;
    }
}

static int start(void)
{
    int wl_enable = cdb_get_int("$wl_enable", 0);

    config_general();

    if ((mtc->rule.OMODE != opMode_p2p)) {
        exec_cmd2("iw dev wlan0 interface add sta0 type managed");
        exec_cmd2("ifconfig sta0 hw ether %s", mtc->rule.STAMAC);
    }
    if (wl_enable == 1) {
        if (mtc->rule.HOSTAPD == 1) {
            mac80211_hostapd_setup_base();
            check_hostapd_terminated();

            MTC_LOGGER(this, LOG_INFO, "%s: Run [%s]", __func__, HOSTAPD_BIN);
            exec_cmd2("start-stop-daemon -S -x %s -- %s 2>/dev/null", HOSTAPD_BIN, HOSTAPD_CMDLINE);
            exec_cmd2("brctl addif %s %s", mtc->rule.BRAP, mtc->rule.AP);
#ifdef HAVE_MULTIPLE_BSSID
            /* Multiple BSSID */
            if (0) {
                int wl_bss_enable3 = cdb_get_int("$wl_bss_enable3", 0);
                int wl_bss_role3 = cdb_get_int("$wl_bss_role3", wlRole_NONE);
                if ((wl_bss_enable3 == 1) && (wl_bss_role3 == wlRole_AP)) {
                    exec_cmd2("brctl addif %s wlan1", mtc->rule.BRAP);
                }
            }
#endif
            hostapd_cli_start();
        }
        if (mtc->rule.WPASUP == 1) {
            if (mtc->rule.OMODE == opMode_p2p) {
                mac80211_p2p_setup_base();
            }
            else {
                mac80211_wpasup_setup_base();
            }

            MTC_LOGGER(this, LOG_INFO, "%s: Run [%s]", __func__, WPASUP_BIN);
            if ((mtc->rule.OMODE >= opMode_wi) && (mtc->rule.OMODE <= opMode_br)) {
                exec_cmd2("start-stop-daemon -S -x "WPASUP_BIN" -- "WPASUP_CMDLINE" "WPASUP_HCTRL, mtc->rule.STA);
            }
            else {
                exec_cmd2("start-stop-daemon -S -x "WPASUP_BIN" -- "WPASUP_CMDLINE, mtc->rule.STA);
            }

            if ((mtc->rule.OMODE != opMode_rp) && (mtc->rule.OMODE != opMode_br) && 
                (mtc->rule.OMODE != opMode_p2p)) {
                exec_cmd2("brctl addif %s %s", mtc->rule.BRSTA, mtc->rule.STA);
            }
            // WISP mode/Pure client mode: monitor link status of station
            if ((mtc->rule.OMODE == opMode_wi) || (mtc->rule.OMODE == opMode_client)) {
                exec_cmd2("start-stop-daemon -S -p %s -x %s -- -u 0 -d 0 -p -q -i %s", 
                          IFPLUGD_STA0_PIDFILE, IFPLUGD_PROC, mtc->rule.STA);
            }
            // br mode: monitor link status of station
            if ((mtc->rule.OMODE == opMode_rp) || (mtc->rule.OMODE == opMode_br)) {
                exec_cmd2("start-stop-daemon -S -p %s -x %s -- -u 0 -d 0 -p -q -i %s", 
                          IFPLUGD_STA0_PIDFILE, IFPLUGD_PROC, mtc->rule.STA);
            }
            // P2P mode call the p2p_cli to handle the p2p progress
            if (mtc->rule.OMODE == opMode_p2p) {
                exec_cmd2("start-stop-daemon -S -x p2p_cli -- -B -p %s", P2P_PIDFILE);
            }
        }
    }

    if (mtc->rule.OMODE != opMode_p2p) {
        exec_cmd("ifconfig sta0 up");
    }

    return 0;
}

static int hostapd_cli_stop(void)
{
    char buf[BUF_LEN_8] = { 0 };
    FILE *fp;
    u8 gpio;

    if( f_exists(HOSTAPD_CLI_ACTIONFILE) ) {
        if((fp = _system_read("sed -n 's/# cdb: //p' "HOSTAPD_CLI_ACTIONFILE" | awk -F';' '{gsub(/.*gpio=/,\"\",$0); print $0}'"))) {
            new_fgets(buf, sizeof(buf), fp);
            _system_close(fp);
        }
        if (buf[0] != 0) {
            gpio = strtoul(buf, NULL, 0); 
            exec_cmd2("echo in > /sys/class/gpio/gpio%d/direction", gpio);
            exec_cmd2("echo %d > /sys/class/gpio/unexport", gpio);
            MTC_LOGGER(this, LOG_INFO, "%s: remove wl_mac_gpio, gpio=[%d]", __func__, gpio);
        }
        unlink(HOSTAPD_CLI_ACTIONFILE);
    }
    if( f_exists(HOSTAPD_CLI_PIDFILE) ) {
        exec_cmd2("start-stop-daemon -K -q -p %s", HOSTAPD_CLI_PIDFILE);
        unlink(HOSTAPD_CLI_PIDFILE);
    }

    return 0;
}

static int stop(void)
{
    hostapd_cli_stop();

#ifdef HAVE_MULTIPLE_BSSID
    /* Multiple BSSID */
    if (0) {
        int wl_bss_enable3 = cdb_get_int("$wl_bss_enable3", 0);
        int wl_bss_role3 = cdb_get_int("$wl_bss_role3", wlRole_NONE);

        if ((wl_bss_enable3 == 1) && (wl_bss_role3 == wlRole_AP)) {
            exec_cmd2("ifconfig wlan1 down");
            exec_cmd2("brctl delif %s wlan1", mtc->rule.BRAP);
        }
    }
#endif

    if ((mtc->rule.AP[0] != 0) && (mtc->rule.BRAP[0] != 0) && f_exists(HOSTAPD_PIDFILE)) {
        exec_cmd2("brctl delif %s %s", mtc->rule.BRAP, mtc->rule.AP);
    }
    if ((mtc->rule.STA[0] != 0) && (mtc->rule.BRSTA[0] != 0)) {
        exec_cmd2("brctl delif %s %s", mtc->rule.BRSTA, mtc->rule.STA);
    }
    if (mtc->rule.HOSTAPD == 1) {
        exec_cmd2("start-stop-daemon -K -q -p %s", HOSTAPD_PIDFILE);
    }
    if (mtc->rule.WPASUP == 1) {
        exec_cmd2("start-stop-daemon -K -q -p %s", WPASUP_PIDFILE);
#ifdef CONFIG_MONTAGE_OIPOD_BOARD
        exec_cmd2("ifconfig %s 0.0.0.0", mtc->rule.BRSTA);
#endif
    }
    if (mtc->rule.OMODE == opMode_p2p) {
        // P2P mode stop the p2p_cli
        exec_cmd2("start-stop-daemon -K -q -p %s", P2P_PIDFILE);
    }
    else {
        exec_cmd2("iw dev sta0 del");
    }
    sleep(1);

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  start },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int wlan_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

