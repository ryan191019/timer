#include <ctype.h>
#include "include/mtc_client.h"
#include "wdk.h"

// wps state
#define WPS_RUN   0
#define WPS_PASS  1
#define WPS_FAIL  2
#define WPS_IDLE  255

// wsc state
#define START     0
#define MESG      1
#define FRAG_ACK  2
#define DONE      4
#define FAIL      5
#define IDLE      255

// wps state
// Enrollee states
#define SEND_M1       0
#define RECV_M2       1
#define SEND_M3       2
#define RECV_M4       3
#define SEND_M5       4
#define RECV_M6       5
#define SEND_M7       6
#define RECV_M8       7
#define RECEIVED_M2D  8
#define WPS_MSG_DONE  9
#define RECV_ACK      10
#define WPS_FINISHED  11
#define SEND_WSC_NACK 12
// Registrar states
#define RECV_M1       13
#define SEND_M2       14
#define RECV_M3       15
#define SEND_M4       16
#define RECV_M5       17
#define SEND_M6       18
#define RECV_M7       19
#define SEND_M8       20
#define RECV_DONE     21
#define SEND_M2D      22
#define RECV_M2D_ACK  23

// Authentication Type Flags
#define AUTH_CAP_OPEN   0x0001
#define AUTH_CAP_SHARED 0x0002 
#define AUTH_CAP_WEP    0x0004
#define AUTH_CAP_WPA    0x0008
#define AUTH_CAP_WPA2   0x0010
#define AUTH_CAP_WPS    0x0020
#define AUTH_CAP_LEAP   0x0040
#define AUTH_CAP_WAPI   0x0080

// Encryption Type Flags
#define AUTH_CAP_CIPHER_NONE         0x0000
#define AUTH_CAP_CIPHER_WEP40        0x0001
#define AUTH_CAP_CIPHER_WEP104       0x0002
#define AUTH_CAP_CIPHER_TKIP         0x0004
#define AUTH_CAP_CIPHER_CCMP         0x0008
#define AUTH_CAP_CIPHER_SMS4         0x0010
#define AUTH_CAP_CIPHER_AES_128_CMAC 0x0020

#define HOSTAPD_CLI_BIN        "/usr/sbin/hostapd_cli"
#define WPS_BSS                1

static char WPS_AP[16] = { 0 };
static char *AP = WPS_AP;

static int wps_callback(char *rbuf, int rlen)
{
    char *ap = strstr(rbuf, "AP=");
    int i = 0, j = 3;

    memset(WPS_AP, 0, sizeof(WPS_AP));
    if (ap && ((rbuf == ap) || (*(ap-1) == '\n'))) {
        while (i < (sizeof(WPS_AP)-1)) {
            WPS_AP[i] = *(ap + j);
            if ((WPS_AP[i] == '\n') || (WPS_AP[i] == 0)) {
                WPS_AP[i] = 0;
                break;
            }
            i++;
            j++;
        }
    }

    return 0;
}

static int __attribute__ ((unused)) wps_get_mtc_info(void)
{
    MtcPkt *packet;
    int ret = RET_ERR;

    if((packet = calloc(1, sizeof(MtcPkt)))) {
        packet->head.ifc = INFC_DAT;
        packet->head.opc = OPC_DAT_INFO;
        ret = mtc_client_call(packet, wps_callback);
        free(packet);
    }

    return ret;
}

static int __attribute__ ((unused)) wps_do_button(void)
{
    wps_get_mtc_info();
    if (*AP) {
        exec_cmd2("%s -i%s wps_pbc", HOSTAPD_CLI_BIN, AP);
        cdb_set_int("$wps_state", IDLE);
        cdb_set_int("$wsc_state", START);
    }

    return 0;
}

static int __attribute__ ((unused)) wps_do_pin(char *args)
{
    char buf[SSBUF_LEN] = { 0 };

    wps_get_mtc_info();
    if ((*AP) && exec_cmd3(buf, sizeof(buf), 
            "%s -i%s wps_check_pin %s", HOSTAPD_CLI_BIN, AP, args) == 0) {
        if (!strstr(buf, "FAIL")) {
            exec_cmd2("%s -i%s wps_pin any %s", HOSTAPD_CLI_BIN, AP, args);
            cdb_set_int("$wps_state", IDLE);
            cdb_set_int("$wsc_state", START);
        }
    }

    return 0;
}

static int __attribute__ ((unused)) wps_do_genpin(void)
{
    char wl_wps_def_pin[SSBUF_LEN] = { 0 };

    wps_get_mtc_info();
    if ((*AP) && exec_cmd3(wl_wps_def_pin, sizeof(wl_wps_def_pin), 
            "%s -i%s wps_ap_pin random", HOSTAPD_CLI_BIN, AP) == 0) {
        if (!strstr(wl_wps_def_pin, "FAIL")) {
            cdb_set("$wl_wps_def_pin", wl_wps_def_pin);
            cdb_commit();
        }
    }

    return 0;
}

static int __attribute__ ((unused)) wps_do_cancel(void)
{
    wps_get_mtc_info();
    if (*AP) {
        return exec_cmd2("%s -i%s wps_cancel", HOSTAPD_CLI_BIN, AP);
    }

    return 0;
}

static char * __attribute__ ((unused)) wps_get_state_type(void)
{
    static char state[16];
    int wps_count_down = cdb_get_int("$wps_count_down", 0);
    int wsc_state = cdb_get_int("$wsc_state", IDLE);
    int wps_state = 0;

    strcpy(state, "Run");

    switch(wsc_state) {
        case FAIL:
            wps_state = cdb_get_int("$wps_state", IDLE);
            if ((wps_state == RECV_DONE) || (wps_state == RECV_ACK) || (wps_state == WPS_MSG_DONE)) {
                strcpy(state, "Pass");
            }
            else {
                strcpy(state, "Fail");
            }
            wps_count_down--;
            cdb_set_int("$wps_count_down", wps_count_down);
            if (wps_count_down <= 0) {
                cdb_set_int("$wps_state", IDLE);
                cdb_set_int("$wsc_state", IDLE);
            }
            break;
        case IDLE:
            strcpy(state, "Idle");
            break;
        case START:
        case MESG:
        case FRAG_ACK:
        case DONE:
        default:
            break;
    }

    return state;
}

static int __attribute__ ((unused)) wps_show_status(void)
{
    //echo "STATE=1,CONF=2,SSID=3,AUTH=4,ENCR=5,KEYIDX=1,KEY=6,UUID=ee,RESULT=Pass"
    char wl_bss_ssid[MSBUF_LEN] = { 0 };
    int wl_bss_sec_type = cdb_get_array_int("$wl_bss_sec_type", WPS_BSS, 0);
    int wl_bss_cipher = cdb_get_array_int("$wl_bss_cipher", WPS_BSS, 0);
    int wl_bss_key_mgt = cdb_get_array_int("$wl_bss_key_mgt", WPS_BSS, 0);
    int wl_bss_wep_index = cdb_get_array_int("$wl_bss_wep_index", WPS_BSS, 0);
    char wl_bss_wep_key[MSBUF_LEN] = { 0 };
    char wl_bss_wpa_psk[MSBUF_LEN] = { 0 };
    int wl_bss_wps_state = cdb_get_array_int("$wl_bss_wps_state", WPS_BSS, 0);

    int auth_cap_open = wl_bss_sec_type & AUTH_CAP_OPEN;
    int auth_cap_shared = wl_bss_sec_type & AUTH_CAP_SHARED;
    int auth_cap_wep = wl_bss_sec_type & AUTH_CAP_WEP;
    int auth_cap_wpa = wl_bss_sec_type & AUTH_CAP_WPA;
    int auth_cap_wpa2 = wl_bss_sec_type & AUTH_CAP_WPA2;
    int auth_cap_cipher_tkip = wl_bss_cipher & AUTH_CAP_CIPHER_TKIP;
    int auth_cap_cipher_ccmp = wl_bss_cipher & AUTH_CAP_CIPHER_CCMP;

    char myIdle[] = "Idle";
    int key_len = 0;
    char *key = NULL;
    char *wps_result = NULL;
    char *wps_status = wps_get_state_type();

    cdb_get_array_str("$wl_bss_ssid", WPS_BSS, wl_bss_ssid, sizeof(wl_bss_ssid), "");
    cdb_get_array_str("$wl_bss_wep_key", wl_bss_wep_index, wl_bss_wep_key, sizeof(wl_bss_wep_key), "");
    cdb_get_array_str("$wl_bss_wpa_psk", WPS_BSS, wl_bss_wpa_psk, sizeof(wl_bss_wpa_psk), "");

    if ((!strcmp(wps_status, "Pass")) || (!strcmp(wps_status, "Fail"))) {
        wps_result = wps_status;
        wps_status = myIdle;
    }
    else {
        wps_result = NULL;
    }

    printf("STATE=%s,", wps_status);
    if (wl_bss_wps_state == 2) {
        printf("CONF=Yes,");
    }
    else {
        printf("CONF=No,");
    }
    printf("SSID=%s,", wl_bss_ssid);
    if (wl_bss_key_mgt == 0) {
        printf("AUTH=");
        if ((auth_cap_wpa > 0) && (auth_cap_wpa2 > 0)) {
            printf("WPAPSK/WPA2PSK,");
        }
        else if (auth_cap_wpa > 0) {
            printf("WPAPSK,");
        }
        else if (auth_cap_wpa2 > 0) {
            printf("WPA2PSK,");
        }
        else if (auth_cap_wep > 0) {
            if (auth_cap_open > 0) {
                printf("Open,");
            }
            else if (auth_cap_shared > 0) {
                printf("shared,");
            }
        }
        else {
            printf(",");
        }

        printf("ENCR=");
        if ((auth_cap_cipher_tkip > 0) && (auth_cap_cipher_ccmp > 0)) {
            printf("TRKP/AES,");
        }
        else if (auth_cap_cipher_tkip > 0) {
            printf("TKIP,");
        }
        else if (auth_cap_cipher_ccmp > 0) {
            printf("AES,");
        }
        else if (auth_cap_wep > 0) {
            printf("WEP,");
        }
        else {
            printf("None,");
        }

        printf("KEYIDX=%d,", wl_bss_wep_index);
        if ((auth_cap_cipher_tkip > 0) || (auth_cap_cipher_ccmp > 0)) {
            key = wl_bss_wpa_psk;
        }
        else if (auth_cap_wep > 0) {
            key = wl_bss_wep_key;
        }
        if (key) {
            key_len = strlen(key);
            printf("Key=");
            while (key_len-- > 0) {
                printf("%x", *key++);
            }
            printf(",");
        }
        else {
            printf("Key=,");
        }

    }
    else {
        printf("AUTH=,ENCR=,KEYIDX=,KEY=,");
    }

    printf("UUID=,RESULT=%s", (wps_result) ? wps_result : "");

    return 0;
}

int wdk_wps(int argc, char **argv)
{
    int err = -1;

    if(argc == 2) {
        if (strcmp("button", argv[1]) == 0) {
            wps_do_button();
            err = 0;
        } else if (strcmp("cancel", argv[1]) == 0)  {
            wps_do_cancel();
            err = 0;
        } else if (strcmp("genpin", argv[1]) == 0)  {
            wps_do_genpin();
            err = 0;
        } else if (strcmp("status", argv[1]) == 0)  {
            wps_show_status();
            err = 0;
        }
    } else if(argc == 3) {
        if (strcmp("pin", argv[1]) == 0) {
            wps_do_pin(argv[2]);
            err = 0;
        }
    }

    if (!err) {
        return 0;
    }

    printf("usage:/lib/wdk/wps [0|1] [button/pin/cancel/genpin/status]\n");
    printf("/lib/wdk/wps [0|1] [pin pincode]\n");

    return -1;
}

