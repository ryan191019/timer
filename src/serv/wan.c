/*=====================================================================================+
 | The definition of wan_mode                                                          |
 +=====================================================================================*/
/* ====================================
  1: static ip
  2: dhcp
  3: pppoe
  4: pptp
  5: l2tp
  6: bigpond (we don't use this now)
  7: pppoe2
  8: pptp2
  9: 3g
===================================== */

#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <unistd.h>
#include <sys/types.h> 
#include <sys/stat.h>

#include <net/ethernet.h>
#include <netutils.h>

#include "include/mtc.h"
#include "include/mtc_misc.h"
#include "include/mtc_proc.h"

#define CCHIP_PATCH

extern MtcData *mtc;
static void check_subnet(void);

int max_alias = 5;
int sys_funcmode = 0;
char dns_def[SSBUF_LEN] = {0};
char dns_fix[SSBUF_LEN] = {0};
char dns_svr1[SSBUF_LEN] = {0};
char dns_svr2[SSBUF_LEN] = {0};
int wan_arpbind_enable = 0;

/*=====================================================================================+
 | Function : wan_serv ip up                                                           |
 +=====================================================================================*/
int mtc_wan_ip_up(void)
{
    char wanif_ip[SSBUF_LEN] = {0};
    char wanif_mac[SSBUF_LEN] = {0};
    char wanif_msk[SSBUF_LEN] = {0};
    char wanif_gw[SSBUF_LEN] = {0};
    char wanif_dns1[SSBUF_LEN] = {0};
    char wanif_dns2[SSBUF_LEN] = {0};
    char lanif_ip[SSBUF_LEN] = {0};
    char lanif_mac[SSBUF_LEN] = {0};
    char usbif_find_eth[SSBUF_LEN] = {0};
    char dns_str[SSBUF_LEN] = {0};
    char dnsrv[SSBUF_LEN] = {0};
    char conntime[4];
    int air_skip = 0;

    cdb_get_int("$airkiss_state", 0);
    if(cdb_get_int("$airkiss_state", 0) == 2) {
       exec_cmd("airkiss apply");
       air_skip = 1;
    }

    exec_cmd("echo \"ip_up:\"`data +\%s` > dev/console");

    switch(mtc->rule.WMODE) {
       case 3:
       case 4:
       case 5:
           if(f_exists("/etc/ppp/resolv.conf"))
              exec_cmd("cp /etc/ppp/resolv.conf /etc/resolv.conf");
           break;

       case 9:
           if(strcmp(cdb_get_str("$usbif_find_eth", usbif_find_eth, sizeof(usbif_find_eth), NULL), "") == 0) {
              if(f_exists("/etc/ppp/resolv.conf"))
                 exec_cmd("cp /etc/ppp/resolv.conf /etc/resolv.conf");
           }
           break;
    }

    if(strcmp(cdb_get_str("$dns_fix", dns_fix, sizeof(dns_fix), NULL), "1") == 0) {
       exec_cmd3(dns_str, sizeof(dns_str), "grep 'nameserver' /etc/resolv.conf");
       exec_cmd("sed -i '//d' /etc/resolv.conf");
       if(strcmp(cdb_get_array_str("$dns_svr", 1, dns_svr1, sizeof(dns_svr1), NULL), "") != 0) {
          MTC_LOGGER(this, LOG_INFO, "dns_svr1=%s", dns_svr1);
          exec_cmd2("echo -e \"nameserver %s\" >> /etc/resolv.conf", dns_svr1);
       }
       if(strcmp(cdb_get_array_str("$dns_svr", 2, dns_svr2, sizeof(dns_svr2), NULL), "") != 0) {
          MTC_LOGGER(this, LOG_INFO, "dns_svr2=%s", dns_svr2);
          exec_cmd2("echo -e \"nameserver %s\" >> /etc/resolv.conf", dns_svr2);
       }
       if(strcmp(dns_str, "") != 0) 
          exec_cmd2("echo -e %s | sed 's/nameserver/\\nnameserver/g' >> /etc/resolv.conf", dns_str);
    }
    else {
       switch(exec_cmd("grep -c 'nameserver' /etc/resolv.conf")) {
         case 0:
             exec_cmd2("echo -e \"nameserver %s\" >> /etc/resolv.conf", dns_def);
             break;

         default:
             break;
       }
    }

    exec_cmd("echo -e \"options timeout:1\noptions attempts:10\" >> /etc/resolv.conf");
    exec_cmd3(dnsrv, sizeof(dnsrv), "awk 'BEGIN{ORD=\" \"} /nameserver/{sub(/nameserver/,\"\"); print $1}' /etc/resolv.conf");

    exec_cmd3(wanif_ip, sizeof(wanif_ip), "ifconfig %s | awk '/ 'addr'/&&!/inet6/{ sub(/^.* 'addr'[:| ]/,\"\"); print $1}'", mtc->rule.WAN);
    cdb_set("$wanif_ip", wanif_ip);
    exec_cmd3(wanif_mac, sizeof(wanif_mac), "ifconfig %s | awk '/ 'HWaddr'/&&!/inet6/{ sub(/^.* 'HWaddr'[:| ]/,\"\"); print $1}'", mtc->rule.WAN);
    cdb_set("$wanif_mac", wanif_mac);
    exec_cmd3(wanif_msk, sizeof(wanif_msk), "ifconfig %s | awk '/ 'Mask'/&&!/inet6/{ sub(/^.* 'Mask'[:| ]/,\"\"); print $1}'", mtc->rule.WAN);
    cdb_set("$wanif_msk", wanif_msk);
    exec_cmd3(wanif_gw, sizeof(wanif_gw), "route -n | awk '/UG/{ print $2 }'");
    cdb_set("$wanif_gw", wanif_gw);
    exec_cmd3(conntime, sizeof(conntime), "/lib/wdk/time");
    cdb_set("$wanif_conntime", conntime);
    exec_cmd3(wanif_dns1, sizeof(wanif_dns1), "echo %s | awk '{print $1}'", dnsrv);
    cdb_set("$wanif_dns1", wanif_dns1);
    exec_cmd3(wanif_dns2, sizeof(wanif_dns2), "echo %s | awk '{print $2}'", dnsrv);
    cdb_set("$wanif_dns2", wanif_dns2);
    exec_cmd3(lanif_ip, sizeof(lanif_ip), "ifconfig %s | awk '/ 'addr'/&&!/inet6/{ sub(/^.* 'addr'[:| ]/,\"\"); print $1}'", mtc->rule.LAN);
    cdb_set("$lanif_ip", lanif_ip);
    exec_cmd3(lanif_mac, sizeof(lanif_mac), "ifconfig %s | awk '/ 'HWaddr'/&&!/inet6/{ sub(/^.* 'HWaddr'[:| ]/,\"\"); print $1}'", mtc->rule.LAN);
    cdb_set("$lanif_mac", lanif_mac);

    if(mtc->rule.OMODE == 0)
       check_subnet();
    if(mtc->rule.OMODE == 3)
       check_subnet();

    /* these following functions have to change from script to C API  */
    /* dhcps (PHASE 2) */
    dnsmasq_main(NULL, "stop");
    dnsmasq_main(NULL, "start");

    /* system (PHASE 2) */
    if(f_exists("/lib/wdk/system"))
       exec_cmd("/lib/wdk/system ntp");

    /* fw */
    if(f_exists("/etc/init.d/fw")) {
       exec_cmd("/etc/init.d/fw stop");
       exec_cmd("/etc/init.d/fw start");
    }

    /* nat */
    if(f_exists("/etc/init.d/nat")) {
       exec_cmd("/etc/init.d/nat stop");
       exec_cmd("/etc/init.d/nat start");
    }

    /* route (PHASE 2) */
    route_main(NULL, "stop");
    route_main(NULL, "start");

    /* lib/wdk/omnicfg_apply IOS, fixed me!!!! */
    //["$FROM_IOS" == "1" -a -n "$REMOTE_ADDR"] && route add "$REMOTE_ADDR" br0

    /* ddns */
    ddns_main(NULL, "stop");
    ddns_main(NULL, "start");

    /* igmpproxy */
    if(f_exists("/etc/init.d/igmpproxy")) {
       exec_cmd("/etc/init.d/igmpproxy stop");
       exec_cmd("/etc/init.d/igmpproxy start");
    }

    /* upnpd */
    upnpd_main(NULL, "stop");
    upnpd_main(NULL, "start");

    /* mpc (PHASE 2) */
    if(f_exists("/usr/bin/mpc")) {
       if(cdb_get_int("$ra_func", 0) == 1)
          exec_cmd("mpc play");
       if((cdb_get_int("$ra_func", 0) == 2) && (air_skip == 0)) {
            shairport_main(NULL, "stop");
            shairport_main(NULL, "start");
        }
    }

    if(mtc->rule.OMODE == 4)
       exec_cmd("wd mat 1");
    if(mtc->rule.OMODE == 5)
       exec_cmd("wd mat 1");

#ifdef CCHIP_PATCH
    if ((mtc->rule.OMODE == OP_BR1_MODE) || (mtc->rule.OMODE == OP_BR2_MODE)) {
        exec_cmd("sed -i 's/ignore_broadcast_ssid=0/ignore_broadcast_ssid=1/' /var/run/hostapd.conf");
				exec_cmd("killall -s 1 hostapd");
    }
#endif

    cdb_set_int("$wanif_state", 2);
    exec_cmd("echo \"ip_up ok:\"`date +\%s` > /dev/console");

    return 0;
}

int mtc_wan_ip_down(void)
{
#ifdef CCHIP_PATCH
    if ((mtc->rule.OMODE == OP_BR1_MODE) || (mtc->rule.OMODE == OP_BR2_MODE)) {
       		exec_cmd("sed -i 's/ignore_broadcast_ssid=1/ignore_broadcast_ssid=0/' /var/run/hostapd.conf");
					exec_cmd("killall -s 1 hostapd");
    }
#endif
    exec_cmd("echo \"ip_down:\"`date +\%s` > /dev/console");
    cdb_set_int("$wanif_state", 0);

    return 0;
}

static int clear_arp_perm(void) // stop() will use this function
{
    char cmd[MAX_COMMAND_LEN];
    FILE *fp;

    sprintf(cmd, "arp -na | awk '/PERM on %s/{gsub(/[()]/,\"\",$2); print $2 }'", mtc->rule.WAN);
    if((fp = _system_read(cmd))) {
        while ( new_fgets(cmd, sizeof(cmd), fp) != NULL) {
            exec_cmd2("arp -d %s > /dev/null 2>&1", cmd);
        }
        _system_close(fp);
    }

    return 0;
}

static int set_arp_wan_bind(void)
{
    char gw[SSBUF_LEN] = {0};

    exec_cmd3(gw, sizeof(gw), "route -n | awk '/UG/{print $2}'");
    exec_cmd2("arp -s %s `arping %s -I %s -c 1 | grep \"Unicast reply\" | awk '{$print 5}' | sed -e s/\\\\\\[// -e s/\\\\\\]//`", gw, gw, mtc->rule.WAN);
    return 0;
}

static void clear_netstatus(void)
{
    unsigned char wanif_mac[ETHER_ADDR_LEN] = { 0 };
    unsigned char lanif_mac[ETHER_ADDR_LEN] = { 0 };
    char wanif_mac_str[9];
    char lanif_mac_str[9];
    int wanif_conntime = cdb_get_int("$wanif_conntime", 0);
    int wanif_conntimes = cdb_get_int("$wanif_conntimes", 0);
    wanif_conntimes = uptime() - wanif_conntime + wanif_conntimes;
    get_ifname_ether_addr(mtc->rule.WAN, wanif_mac, sizeof(wanif_mac));
    get_ifname_ether_addr(mtc->rule.LAN, lanif_mac, sizeof(lanif_mac));
    my_val2mac(wanif_mac_str, wanif_mac);
    my_val2mac(lanif_mac_str, lanif_mac);

MTC_LOGGER(this, LOG_INFO, "uptime =%ld", uptime());
MTC_LOGGER(this, LOG_INFO, "wan =%s", mtc->rule.WAN);
MTC_LOGGER(this, LOG_INFO, "wan mac = [%s]", wanif_mac_str);
MTC_LOGGER(this, LOG_INFO, "lan = %s", mtc->rule.LAN);
MTC_LOGGER(this, LOG_INFO, "lan mac = [%s]", lanif_mac_str);

    cdb_set("$wanif_mac", wanif_mac_str);
    cdb_set("$wanif_ip", "0.0.0.0");
    cdb_set("$wanif_msk", "0.0.0.0");
    cdb_set("$wanif_gw", "0.0.0.0");
    cdb_set("$wanif_domain", "");
    cdb_set_int("$wanif_conntime", 0);
    cdb_set_int("$wanif_conntimes", wanif_conntimes);
    cdb_set("$wanif_dns1", "0.0.0.0");
    cdb_set("$wanif_dns2", "0.0.0.0");
    cdb_set("$lanif_mac", lanif_mac_str);
}

static void save_pppoe_file(void)
{
    char poe_user[SSBUF_LEN] = {0};
    char poe_pass[SSBUF_LEN] = {0};
    char poe_sip[SSBUF_LEN] = {0};
    char poe_svc[SSBUF_LEN] = {0};
    int poe_mtu = 0;
    int poe_idle = 0;
    char string[LSBUF_LEN] = {0};
    char *str;
    FILE *fp;

    cdb_get_str("$poe_user", poe_user, sizeof(poe_user), NULL);
    cdb_get_str("$poe_pass", poe_pass, sizeof(poe_pass), NULL);
    cdb_get_int("$poe_sipe", 0);
    cdb_get_str("$poe_sip", poe_sip, sizeof(poe_sip), NULL);
    cdb_get_str("$poe_svc", poe_svc, sizeof(poe_svc), NULL);
    cdb_get_int("$poe_mtu", 0);
    cdb_get_int("$poe_dile", 0);
    cdb_get_int("$poe_auto", 0);

		mkdir("/etc/ppp", 0755);
    if(!f_isdir("/etc/ppp/peers")) {
    	mkdir("/etc/ppp/peers", 0755);
    }

    unlink("/etc/ppp/peers/ppp0");
    unlink("/etc/ppp/pap-secrets");
    unlink("/etc/ppp/chap-secrets");
    unlink("/etc/ppp/resolv.conf");

    /* manual_onoff is in phase 2, ignore */

    fp = fopen("/etc/ppp/chap-secrets", "w+");
    if(fp) {
       fprintf(fp, "#USERNAME  PROVIDER  PASSWORD  IPADDRESS\n");
       fprintf(fp, "\"%s\" * \"%s\" *\n", poe_user, poe_pass);
    }
    fclose(fp);
    exec_cmd("cp /etc/ppp/chap-secrets /etc/ppp/pap-secrets");

    str = string;
    str += sprintf(str, "plugin rp-pppoe.so\n");
   
    if(strcmp(cdb_get_str("$poe_svc", poe_svc, sizeof(poe_svc), NULL), "") != 0)
       str += sprintf(str, "rp_pppoe_service \"%s\"\n", poe_svc);

    str += sprintf(str, "eth0.0\n");
    str += sprintf(str, "noipdefault\n");
    str += sprintf(str, "ipcp-accept-local\n");
    str += sprintf(str, "ipcp-accept-remote\n");
    str += sprintf(str, "defaultroute\n");
    str += sprintf(str, "replacedefaultroute\n");
    str += sprintf(str, "hide-password\n");
    str += sprintf(str, "noauth\n");
    str += sprintf(str, "refuse-eap\n");
    str += sprintf(str, "lcp-echo-interval 30\n");
    str += sprintf(str, "lcp-echo-failure 4\n");

    if(strcmp(cdb_get_str("$dns_fix", dns_fix, sizeof(dns_fix), NULL), "0") == 0)
       str += sprintf(str, "usepeerdns\n");
    else {
       if(strcmp(cdb_get_array_str("$dns_svr", 1, dns_svr1, sizeof(dns_svr1), NULL), "") != 0)
          str += sprintf(str, "ms-dns %s", dns_svr1);
       if(strcmp(cdb_get_array_str("$dns_svr", 2, dns_svr2, sizeof(dns_svr2), NULL), "") != 0)
          str += sprintf(str, "ms-dns %s", dns_svr2);
    }

    str += sprintf(str, "user \"%s\"\n", poe_user);

    if(cdb_get_int("$poe_sipe", 0) == 1)
       str += sprintf(str, "%s:\n", poe_sip);

    str += sprintf(str, "mtu %d\n", poe_mtu);
    //str += sprintf(str, "mtu 1492\n");

    if(cdb_get_int("$poe_idle", 0) != 0) {
       switch(cdb_get_int("$poe_auto", 0)) {
           case 0:
               str += sprintf(str, "persist\n");
               break;
           case 1:
               str += sprintf(str, "demand\n");
               break;
       }
       if(cdb_get_int("$poe_auto", 0) != 0)
          str += sprintf(str, "idle %d\n", poe_idle);
    }
    else
       str += sprintf(str, "persist\n");

    fp = fopen("/etc/ppp/peers/ppp0", "w+");
    if(fp) {
       fprintf(fp, "%s", string);
    }
    fclose(fp);
}

static void save_pptp_file(void)
{
    char pptp_user[SSBUF_LEN] = {0};
    char pptp_pass[SSBUF_LEN] = {0};
    char pptp_svr[SSBUF_LEN] = {0};
    int pptp_mtu = 0;
    int pptp_idle = 0;
    char string[LSBUF_LEN] = {0};
    char *str;
    FILE *fp;

    cdb_get_str("$pptp_user", pptp_user, sizeof(pptp_user), NULL);
    cdb_get_str("$pptp_pass", pptp_pass, sizeof(pptp_pass), NULL);
    cdb_get_str("$pptp_svr", pptp_svr, sizeof(pptp_svr), NULL);
    cdb_get_int("$pptp_mtu", 0);
    cdb_get_int("$pptp_dile", 0);
    cdb_get_int("$pptp_auto", 0);
    cdb_get_int("$pptp_mppe", 0);

		mkdir("/etc/ppp", 0755);
    if(!f_isdir("/etc/ppp/peers")) {
       mkdir("/etc/ppp/peers", 0755);
    }

    unlink("/etc/ppp/peers/ppp0");
    unlink("/etc/ppp/chap-secrets");
    unlink("/etc/ppp/resolv.conf");

    /* manual_onoff is in phase 2, ignore */

    fp = fopen("/etc/ppp/chap-secrets", "w+");
    if(fp) {
       fprintf(fp, "#USERNAME  PROVIDER  PASSWORD  IPADDRESS\n");
       fprintf(fp, "\"%s\" * \"%s\" *\n", pptp_user, pptp_pass);
    }
    fclose(fp);

    str = string;
    str += sprintf(str, "pty \"pptp %s --nolaunchpppd\"\n", pptp_svr);
    str += sprintf(str, "noipdefault\n");
    str += sprintf(str, "ipcp-accept-local\n");
    str += sprintf(str, "ipcp-accept-remote\n");
    str += sprintf(str, "defaultroute\n");
    str += sprintf(str, "replacedefaultroute\n");
    str += sprintf(str, "hide-password\n");
    str += sprintf(str, "noauth\n");
    str += sprintf(str, "refuse-eap\n");
    str += sprintf(str, "lcp-echo-interval 30\n");
    str += sprintf(str, "lcp-echo-failure 4\n");

    if(cdb_get_str("$dns_fix", dns_fix, sizeof(dns_fix), NULL) == 0)
       str += sprintf(str, "usepeerdns\n");
    else {
       if(cdb_get_array_str("$dns_svr", 1, dns_svr1, sizeof(dns_svr1), NULL) != NULL)
          str += sprintf(str, "ms-dns %s\n", dns_svr1);
       if(cdb_get_array_str("$dns_svr", 2, dns_svr2, sizeof(dns_svr2), NULL) != NULL)
          str += sprintf(str, "ms-dns %s\n", dns_svr2);
    }

    if(cdb_get_int("$pptp_mppe", 0) == 1) {
       str += sprintf(str, "mppe required\n");
       str += sprintf(str, "mppe stateless\n");
    }

    str += sprintf(str, "user \"%s\"\n", pptp_user);
    str += sprintf(str, "mtu %d\n", pptp_mtu);

    if(cdb_get_int("$pptp_idle", 0) != 0) {
       switch(cdb_get_int("$pptp_auto", 0)) {
          case 0:
              str += sprintf(str, "persist\n");
              break;
          case 1:
              str += sprintf(str, "demand\n");
              break;
       }
       if(!cdb_get_int("$pptp_auto", 0) == 0)
          str += sprintf(str, "idle %d\n", pptp_idle);
    }
    else
       str += sprintf(str, "persist\n");

    fp = fopen("/etc/ppp/peers/ppp0", "w+");
    if(fp) {
       fprintf(fp, "%s", string);
    }
    fclose(fp);    
}

static void save_l2tp_file(void)
{
   char l2tp_user[SSBUF_LEN] = {0};
   char l2tp_pass[SSBUF_LEN] = {0};
   char l2tp_svr[SSBUF_LEN] = {0};
   int l2tp_mtu = 0;
   int l2tp_idle = 0;
   char *str1, *str2;
   char string1[LSBUF_LEN] = {0};
   char string2[LSBUF_LEN] = {0};
   FILE *fp;

   cdb_get_str("$l2tp_user", l2tp_user, sizeof(l2tp_user), NULL);
   cdb_get_str("$l2tp_pass", l2tp_pass, sizeof(l2tp_pass), NULL);
   cdb_get_str("$l2tp_svr", l2tp_svr, sizeof(l2tp_svr), NULL);
   cdb_get_int("$l2tp_mtu", 0);
   cdb_get_int("$l2tp_idle", 0);
   cdb_get_int("$l2tp_auto", 0);
   cdb_get_int("$l2tp_mppe", 0);

   if(!f_isdir("/etc/ppp/peers")) {
       mkdir("/etc/ppp/peers", 0755);
   }

    unlink("/etc/l2tp.conf");
    unlink("/etc/ppp/options");
    unlink("/etc/ppp/chap-secrets");
    unlink("/etc/ppp/resolv.conf");

    /* manual_onoff is in phase 2, ignore */

    fp = fopen("/etc/ppp/chap-secrets", "w+");
    if(fp) {
       fprintf(fp, "#USERNAME  PROVIDER  PASSWORD  IPADDRESS\n");
       fprintf(fp, "\"%s\" * \"%s\" *\n", l2tp_user, l2tp_pass);
    }
    fclose(fp);

    str1 = string1;
    str1 += sprintf(str1, "global\n");
    str1 += sprintf(str1, "load-handler \"sync-pppd.so\"\n");
    str1 += sprintf(str1, "load-handler \"cmd.so\"\n");
    str1 += sprintf(str1, "listen-port 1701\n");
    str1 += sprintf(str1, "section sync-pppd\n");
    str1 += sprintf(str1, "lac-pppd-opts \"file /etc/ppp/options\"\n");
    str1 += sprintf(str1, "section peer\n");
    str1 += sprintf(str1, "peer %s\n", l2tp_svr);
    str1 += sprintf(str1, "port 1701\n");
    str1 += sprintf(str1, "lac-handler sync-pppd\n");
    str1 += sprintf(str1, "lns-handler sync-pppd\n");
    str1 += sprintf(str1, "hide-avps yes\n");
    str1 += sprintf(str1, "section cmd\n");

    fp = fopen("/etc/l2tp.conf", "w+");
    if(fp) {
       fprintf(fp, "%s", string1);
    }
    fclose(fp); 

    str2 = string2;
    str2 += sprintf(str2, "noipdefault\n");
    str2 += sprintf(str2, "ipcp-accept-local\n");
    str2 += sprintf(str2, "ipcp-accept-remote\n");
    str2 += sprintf(str2, "defaultroute\n");
    str2 += sprintf(str2, "replacedefaultroute\n");
    str2 += sprintf(str2, "hide-password\n");
    str2 += sprintf(str2, "noauth\n");
    str2 += sprintf(str2, "refuse-eap\n");
    str2 += sprintf(str2, "lcp-echo-interval 30\n");
    str2 += sprintf(str2, "lcp-echo-failure 4\n");

    if(cdb_get_str("$dns_fix", dns_fix, sizeof(dns_fix), NULL) == 0)
       str2 += sprintf(str2, "usepeerdns\n");
    else {
       if(cdb_get_array_str("$dns_svr", 1, dns_svr1, sizeof(dns_svr1), NULL) != NULL)
          str2 += sprintf(str2, "ms-dns %s\n", dns_svr1);
       if(cdb_get_array_str("$dns_svr", 2, dns_svr2, sizeof(dns_svr2), NULL) != NULL)
          str2 += sprintf(str2, "ms-dns %s\n", dns_svr2);
    }

    if(cdb_get_int("$l2tp_mppe", 0) == 1) {
       str2 += sprintf(str2, "mppe required\n");
       str2 += sprintf(str2, "mppe stateless\n");
    }

    str2 += sprintf(str2, "user \"%s\"\n", l2tp_user);
    str2 += sprintf(str2, "mtu %d\n", l2tp_mtu);

    if(cdb_get_int("$l2tp_idle", 0) != 0) {
       switch(cdb_get_int("$l2tp_auto", 0)) {
          case 0:
              str2 += sprintf(str2, "persist\n");
              break;
          case 1:
              str2 += sprintf(str2, "demand\n");
              break;
       }
       if(!cdb_get_int("$l2tp_auto", 0) == 0)
          str2 += sprintf(str2, "idle %d\n", l2tp_idle);
    }
    else
       str2 += sprintf(str2, "persist\n");

    fp = fopen("/etc/ppp/options", "w+");
    if(fp) {
       fprintf(fp, "%s", string2);
    }
    fclose(fp);
}

static void save_3gmodem_file(void)
{
    char wan_mobile_apn[SSBUF_LEN] = {0};
    char wan_mobile_dial[SSBUF_LEN] = {0};
    int wan_mobile_idle = 0;
    int wan_mobile_mtu = 0;
    char wan_mobile_pass[SSBUF_LEN] = {0};
    char wan_mobile_pin[SSBUF_LEN] = {0};
    char wan_mobile_user[SSBUF_LEN] = {0};
    char usbif_modemtty[SSBUF_LEN] = {0};
    char usbif_telecom[SSBUF_LEN] = {0};
    char string[LSBUF_LEN] = {0};
    FILE *fp;
    char *str;
    char mcc_mnc[SSBUF_LEN] = {0};
    char line[SSBUF_LEN] = {0};
    char pinstatus[SSBUF_LEN] = {0};

    cdb_get_str("$wan_mobile_apn", wan_mobile_apn, sizeof(wan_mobile_apn), NULL);
    cdb_get_int("$wan_mobile_auto", 0);
    cdb_get_int("$wan_mobile_autopan", 0);
    cdb_get_str("$wan_mobile_dial", wan_mobile_dial, sizeof(wan_mobile_dial), NULL);
    cdb_get_int("$wan_mobile_idle", 0);
    cdb_get_int("$wan_mobile_ispin", 0);
    cdb_get_int("$wan_mobile_mtu", 0);
    cdb_get_int("$wan_mobile_optime", 0);
    cdb_get_str("$wan_mobile_pass", wan_mobile_pass, sizeof(wan_mobile_pass), NULL);
    cdb_get_str("$wan_mobile_pin", wan_mobile_pin, sizeof(wan_mobile_pin), NULL);
    cdb_get_str("$wan_mobile_user", wan_mobile_user, sizeof(wan_mobile_user), NULL);
    cdb_get_str("$usbif_modemtty", usbif_modemtty, sizeof(usbif_modemtty), NULL);
    cdb_get_str("$usbif_telecom", usbif_telecom, sizeof(usbif_telecom), NULL);

    if((strcmp(cdb_get_str("$usbif_modemtty", usbif_modemtty, sizeof(usbif_modemtty), NULL), "!ERR") == 0) || (cdb_get_str("$usbif_modemtty", usbif_modemtty, sizeof(usbif_modemtty), NULL) == NULL))
       return;

    if(!f_isdir("/etc/ppp/peers")) {
       mkdir("/etc/ppp/peers", 0755);
    }

    unlink("/etc/ppp/peers/ppp0");
    unlink("/etc/ppp/pap-secrets");
    unlink("/etc/ppp/chap-secrets");

    /* manual_onoff is in phase 2, ignore */

    exec_cmd3(pinstatus, sizeof(pinstatus), "`/usr/bin/comgt -d /dev/%s -s /etc/gcom/getpin.conf`", usbif_modemtty);
    if((strcmp(pinstatus, "READY") != 0) && (strcmp(pinstatus, "SIN PIN") != 0)) {
       exec_cmd2("echo \"[3G modem]!!!!!Unsupported cases; PIN's error status is %s > /dev/kmsg", pinstatus);
       exec_cmd2("echo \"[3G modem]cmd:%s > /dev/kmsg", pinstatus);

       while(strcmp(pinstatus, "SIM busy") == 0) {
           f_write_string("/dev/kmsg", "[3G modem]get pinstatus again\n" , 0, 0);
           sleep(1);
       }
       
       f_wsprintf("/dev/kmsg", "[3G modem]pinstatus is %s\n" , pinstatus);
       
       if((strcmp(pinstatus, "READY") != 0) && (strcmp(pinstatus, "SIN PIN") != 0))
          return;
    }

    if(strcmp(pinstatus, "SIM PIN") == 0) {
       cdb_set_int("$wan_mobile_ispin", 1);
       if(cdb_get_str("$wan_mobile_pin", wan_mobile_pin, sizeof(wan_mobile_pin), NULL) != NULL) {
          exec_cmd2("export \"PINCODE=%s\"", wan_mobile_pin);
          exec_cmd2("/usr/bin/comgt -d /dev/%s -s /etc/gcom/setpin.gcom", usbif_modemtty);
		  if(exec_cmd2("/usr/bin/comgt -d /dev/%s -s /etc/gcom/setpin.gcom", usbif_modemtty) == 1) {
             f_write_string("/dev/kmsg", "[3G modem]set pin failed\n" , 0, 0);
             return;
          }
       }
    }
    else
       cdb_set_int("$wan_mobile_ispin", 0);

    exec_cmd3(pinstatus, sizeof(pinstatus), "`/usr/bin/comgt -d /dev/%s -s /etc/gcom/getpin.conf`", usbif_modemtty);
    if(strcmp(pinstatus, "READY") != 0)
       return;

    exec_cmd3(mcc_mnc, sizeof(mcc_mnc), "/usr/bin/comgt -d /dev/%s -s  /etc/gcom/getmccmnc.gcom", usbif_modemtty);
    exec_cmd3(line, sizeof(line), "sed -n \"/^%s/p\" /etc/config/apnlist.lst", mcc_mnc); // grep string "mcc_mnc" from file /etc/config/apnlist.lst

    if((cdb_get_str("$usbif_telecom", usbif_telecom, sizeof(usbif_telecom), NULL) == NULL) || (strcmp(cdb_get_str("$usbif_telecom", usbif_telecom, sizeof(usbif_telecom), NULL), "!ERR") == 0)) {
       cdb_set("$usbif_telecom", (void *)exec_cmd2("echo %s | awk -F, '{ print $6}'", line));
    }

    if(cdb_set_int("$wan_mobile_autopan", 0) == 1) {
       exec_cmd3(wan_mobile_dial, sizeof(wan_mobile_dial), "echo %s | awk -F, '{ print $2}'", line);
       exec_cmd3(wan_mobile_apn, sizeof(wan_mobile_apn), "echo %s | awk -F, '{ print $3}'", line);
       exec_cmd3(wan_mobile_user, sizeof(wan_mobile_user), "echo %s | awk -F, '{ print $4}'", line);
       exec_cmd3(wan_mobile_pass, sizeof(wan_mobile_pass), "echo %s | awk -F, '{ print $5}'", line);
    }

    fp = fopen("/etc/chatscripts/3g.chat", "w");
    if(fp) {
       fprintf(fp, "TIMEOUT 10\n");
       fprintf(fp, "ABORT BUSY\n");
       fprintf(fp, "ABORT ERROR");
       fprintf(fp, "REPORT CONNECT\n");
       fprintf(fp, "ABORT NO CARRIER\n");
       fprintf(fp, "ABORT VOICE\n");
       fprintf(fp, "ABORT \"NO DIALTONE\"\n");
       fprintf(fp, "\"\" 'at+cgdcont=1,\"ip\",\"%s\"'\n", wan_mobile_apn);
       fprintf(fp, "\"\" \"atd%s\"\n", wan_mobile_dial);
       fprintf(fp, "TIMEOUT 10\n");
       fprintf(fp, "CONNECT c\n");
       fclose(fp);
    }

    str = string;
    str += sprintf(str, "/dev/%s\n", usbif_modemtty);
    str += sprintf(str, "crtscts\n");
    str += sprintf(str, "noauth\n");
    str += sprintf(str, "defaultroute\n");
    str += sprintf(str, "noipdefault\n");
    str += sprintf(str, "nopcomp\n");
    str += sprintf(str, "nocacomp\n");
    str += sprintf(str, "novj\n");
    str += sprintf(str, "nobsdcomp\n");
    str += sprintf(str, "holdoff 10\n");
    str += sprintf(str, "nodeflate\n");

    if(cdb_get_str("$dns_fix", dns_fix, sizeof(dns_fix), NULL) == 0)
       str += sprintf(str, "usepeerdns\n");
    else {
       if(cdb_get_array_str("$dns_svr", 1, dns_svr1, sizeof(dns_svr1), NULL) != NULL)
          str += sprintf(str, "ms-dns %s", dns_svr1);
       if(cdb_get_array_str("$dns_svr", 2, dns_svr2, sizeof(dns_svr2), NULL) != NULL)
          str += sprintf(str, "ms-dns %s", dns_svr2);
    }

    str += sprintf(str, "user \"%s\"\n", wan_mobile_user);
    str += sprintf(str, "password \"%s\"", wan_mobile_pass);
    str += sprintf(str, "mtu %d\n", wan_mobile_mtu);

    if(cdb_get_int("$wan_mobile_idle", 0) != 0) {
       switch(cdb_get_int("$wan_mobile_auto", 0)) {
          case 0:
              str += sprintf(str, "persist\n");
              break;
          case 1:
              str += sprintf(str, "demand\n");
              break;
       }
       if(!cdb_get_int("$wan_mobile_auto", 0) == 0)
          str += sprintf(str, "idle %d\n", wan_mobile_idle);
    }
    else
       str += sprintf(str, "persist\n");

    str += sprintf(str, "connect \"/usr/sbin/chat -v -r /var/log/chat.log -f /etc/chatscripts/3g.chat\"\n");

    fp = fopen("/etc/ppp/peers/ppp0", "w+");
    if(fp) {
       fprintf(fp, "%s", string);
    }
    fclose(fp);
}

static int dialup(void)
{
    if(f_exists("/tmp/cdc-wdm")) {
       int wan_mobile_auto = 0;
       //int wan_mobile_autoapn = 0;
       char wan_mobile_apn[SSBUF_LEN] = {0};
       char wan_mobile_pass[SSBUF_LEN] = {0};
       char wan_mobile_user[SSBUF_LEN] = {0};
       char mcc_mnc[SSBUF_LEN] = {0};
       char line[SSBUF_LEN] = {0};
       char APN[SSBUF_LEN] = {0};
       char PASS[SSBUF_LEN] = {0};
       char USER[SSBUF_LEN] = {0};

       cdb_get_int("$wan_mobile_auto", 0);
       cdb_get_int("$wan_mobile_autopan", 0);

       if(cdb_get_int("$wan_mobile_autopan", 0) == 1) {
          exec_cmd3(mcc_mnc, sizeof(mcc_mnc), "uqmi -sd cat /tmp/cdc-wdm --get-serving-system|awk -F, '{print $2\":\"$3}'|awk -F: '{print $2$4}'");
          exec_cmd3(line, sizeof(line), "sed -n \"/^%s/p\" /etc/config/apnlist.lst", mcc_mnc);
          exec_cmd3(APN, sizeof(APN), "echo %s | awk -F, '{ print $3}'", line);
          exec_cmd3(USER, sizeof(USER), "echo %s | awk -F, '{ print $4}'", line);
          exec_cmd3(PASS, sizeof(PASS), "echo %s | awk -F, '{ print $5}'", line);
       }
       else {
          cdb_get_str("$wan_mobile_apn", APN, sizeof(wan_mobile_apn), NULL);
          cdb_get_str("$wan_mobile_pass", PASS, sizeof(wan_mobile_pass), NULL);
          cdb_get_str("$wan_mobile_user", USER, sizeof(wan_mobile_user), NULL);
       }

       if(APN != NULL) {
          exec_cmd2("qmi start cat /tmp/cdc-wdm %s %s %s %s", APN, wan_mobile_auto, PASS, USER);
       }
       else {
          MTC_LOGGER(this, LOG_ERR, "echo \"dialup fail: no APN\"\n");
          return 1;
       }
    }
    return 0;
}

static void start_dhcp(void)
{
    int wan_dhcp_mtu = 0;
    char wan_dhcp_reqip[SSBUF_LEN] = {0};

    //MTC_LOGGER(this, LOG_INFO, "START DHCP");
    cdb_get_int("$wan_dhcp_mtu", 0);
    cdb_get_str("$wan_dhcp_reqip", wan_dhcp_reqip, sizeof(wan_dhcp_reqip), NULL);

    exec_cmd2("ifconfig %s mtu %d up", mtc->rule.WANB, wan_dhcp_mtu);
    //MTC_LOGGER(this, LOG_INFO, "wanb = %s, mtu = %d, reqip = %s", mtc->rule.WANB, wan_dhcp_mtu, wan_dhcp_reqip);

    if(cdb_get_str("$wan_dhcp_reqip", wan_dhcp_reqip, sizeof(wan_dhcp_reqip), NULL) != NULL)
       exec_cmd2("/sbin/dhcpcd %s -s %s -t 0 >/dev/null 2>/dev/null &", mtc->rule.WANB, wan_dhcp_reqip);
    else
       exec_cmd2("/sbin/dhcpcd %s -t 0 >/dev/null 2>/dev/null &", mtc->rule.WANB);
}

static void start_ppp(void)
{
    //MTC_LOGGER(this, LOG_INFO, "startpppp");
    exec_cmd("/usr/sbin/pppd call ppp0 >/dev/null 2>/dev/null &");
}

static void check_subnet(void)
{
    char lan_msk[SSBUF_LEN] = {0};
    char lan_ip[SSBUF_LEN] = {0};
    char wanif_msk[SSBUF_LEN] = {0};
    char wanif_ip[SSBUF_LEN] = {0};
    char lan_subnet[SSBUF_LEN] = {0};
    char wan_subnet[SSBUF_LEN] = {0};

    cdb_get_str("$lan_msk", lan_msk, sizeof(lan_msk), NULL);
    cdb_get_str("$wanif_msk", wanif_msk, sizeof(wanif_msk), NULL);

    if(strcmp(cdb_get_str("$lan_msk", lan_msk, sizeof(lan_msk), NULL), cdb_get_str("$wanif_msk", wanif_msk, sizeof(wanif_msk), NULL)) == 0) {
       cdb_get_str("$lan_ip", lan_ip, sizeof(lan_ip), NULL);
       cdb_get_str("$wanif_ip", wanif_ip, sizeof(wanif_ip), NULL);

       exec_cmd3(lan_subnet, sizeof(lan_subnet), "echo %s %s | awk -F'[ .]' '{print and($1,$5)\".\"and($2,$6)\".\"and($3,$7)\".\"and($4,$8)}'", lan_ip, lan_msk);
       exec_cmd3(wan_subnet, sizeof(wan_subnet), "echo %s %s | awk -F'[ .]' '{print and($1,$5)\".\"and($2,$6)\".\"and($3,$7)\".\"and($4,$8)}'", wanif_ip, wanif_msk);

       if(strcmp(lan_subnet, wan_subnet) == 0) {
          cdb_set("$lan_msk","255.255.255.0");

          if(strcmp(lan_subnet, "192.168.0.0") == 0)
             cdb_set("$lan_ip", "192.168.169.1");
          else
             cdb_set("$lan_ip", "192.168.0.1");
          
          cdb_set_int("$smrt_change", 1);
          exec_cmd("/lib/wdk/commit");
       }
    }
}

static void static_ip_up(void)
{
    char wan_ip[SSBUF_LEN] = {0};
    char wan_msk[SSBUF_LEN] = {0};
    char wan_gw[SSBUF_LEN] = {0};
    char wan_alias_ip[SSBUF_LEN] = {0};
    int wan_mtu = 0;
    int wan_alias = 0;
    int inum = 1;

    cdb_get_str("$wan_ip", wan_ip, sizeof(wan_ip), NULL);
    cdb_get_str("$wan_msk", wan_msk, sizeof(wan_msk), NULL);
    cdb_get_str("$wan_gw", wan_gw, sizeof(wan_gw), NULL);
    cdb_get_int("$wan_mtu", 0);
    cdb_get_int("$wan_alias", 0);

    if(cdb_get_str("$wan_ip", wan_ip, sizeof(wan_ip), NULL) == NULL)
       MTC_LOGGER(this, LOG_ERR, "ip is NULL, error");
    if(cdb_get_str("$wan_msk", wan_msk, sizeof(wan_msk), NULL) == NULL)
       MTC_LOGGER(this, LOG_ERR, "netmask is NULL, error");
    if(cdb_get_str("$wan_gw", wan_gw, sizeof(wan_gw), NULL) == NULL)
       MTC_LOGGER(this, LOG_ERR, "gateway is NULL, error");

    MTC_LOGGER(this, LOG_INFO, "static ip up:");
    exec_cmd2("ifconfig %s %s netmask %s mtu %d up", mtc->rule.WANB, wan_ip, wan_msk, wan_mtu);
    exec_cmd2("route add default gw %s", wan_gw);

    if(wan_alias == 1) {
       while(inum <= max_alias) {
          cdb_get_array_str("$wan_alias_ip", inum, wan_alias_ip, sizeof(wan_alias_ip), NULL);

          if(cdb_get_array_str("$wan_alias_ip", inum, wan_alias_ip, sizeof(wan_alias_ip), NULL) != NULL)
             exec_cmd2("ifconfig %s:%d `config_chk %s` netmask %s", mtc->rule.WANB, inum, wan_alias_ip, wan_msk);
          
          inum = inum+1;
       }
    }
   
    //MTC_LOGGER(this, LOG_INFO, "static ip up!!!") ;
    mtc_wan_ip_up();
    cdb_set_int("$wanif_state", 2);
}

static void pptp_up(void)
{
    char pptp_ip[SSBUF_LEN] = {0};
    char pptp_msk[SSBUF_LEN] = {0};
    char pptp_gw[SSBUF_LEN] = {0};
    char pptp_id[SSBUF_LEN] = {0};
    
    cdb_get_int("$pptp_if_mode", 0);
    cdb_get_str("$pptp_ip", pptp_ip, sizeof(pptp_ip), NULL);
    cdb_get_str("$pptp_msk", pptp_msk, sizeof(pptp_msk), NULL);
    cdb_get_str("$pptp_gw", pptp_gw, sizeof(pptp_gw), NULL);
    cdb_get_str("$pptp_id", pptp_id, sizeof(pptp_id), NULL);
    
    switch(cdb_get_int("$pptp_if_mode", 0)) {
       case 0:
           exec_cmd2("ifconfig %s %s netmask %s up", mtc->rule.WANB, pptp_ip, pptp_msk);
           exec_cmd2("route add default gw %s", pptp_gw);
           break;

       case 1:
           start_dhcp();
           break;
    }
    save_pptp_file();
    start_ppp();
}

static void l2tp_up(void)
{
    char l2tp_ip[SSBUF_LEN] = {0};
    char l2tp_msk[SSBUF_LEN] = {0};
    char l2tp_gw[SSBUF_LEN] = {0};
    char l2tp_id[SSBUF_LEN] = {0};
    char l2tp_svr[SSBUF_LEN] = {0};
           
    cdb_get_int("$l2tp_if_mode", 0);
    cdb_get_str("$l2tp_ip", l2tp_ip, sizeof(l2tp_ip), NULL);
    cdb_get_str("$l2tp_msk", l2tp_msk, sizeof(l2tp_msk), NULL);
    cdb_get_str("$l2tp_gw", l2tp_gw, sizeof(l2tp_gw), NULL);
    cdb_get_str("$l2tp_id", l2tp_id, sizeof(l2tp_id), NULL);
    cdb_get_str("$l2tp_svr", l2tp_svr, sizeof(l2tp_svr), NULL);
    
    switch(cdb_get_int("$l2tp_if_mode", 0)) {
       case 0:
           exec_cmd2("ifconfig %s %s netmask %s up", mtc->rule.WANB, l2tp_ip, l2tp_msk);
           exec_cmd2("route add default gw %s", l2tp_gw);
           break;
  
       case 1:
           start_dhcp();
           break;
    }
    save_l2tp_file();
    exec_cmd("/usr/sbin/l2tpd");
    exec_cmd2("/usr/sbin/l2tp-control \"start-session %s\" >/dev/null 2>/dev/null",l2tp_svr);
}

static int start(void)
{
    unsigned char mac[ETHER_ADDR_LEN];
    char ip[30];
    char out[30];
    char usbif_find_eth[SSBUF_LEN] = {0};
    char str[SSBUF_LEN];

    get_ifname_ether_addr("br0", mac, sizeof(mac));
    MTC_LOGGER(this, LOG_INFO, "br0 mac =%s", ether_etoa(mac, out));
    
    get_ifname_ether_ip("br0", ip, sizeof(ip));
    MTC_LOGGER(this, LOG_INFO, "br0 ip =%s", ip);

    cdb_get_int("$wan_mode", 0);
    cdb_get_str("$usbif_find_eth", usbif_find_eth, sizeof(usbif_find_eth), NULL);
    cdb_get_int("$wanif_state", 0);

    if((mtc->rule.IFPLUGD == NULL) && (mtc->rule.WMODE) != 9) {
       MTC_LOGGER(this, LOG_INFO, "wan start fail: without ifplugd interface");
       return 0;
    }

    if(mtc->rule.WAN == NULL) {
       MTC_LOGGER(this, LOG_INFO, "echo \"wan start fail: null wan interface(%s)\"", mtc->rule.WAN);
       return 0;
    }

    //MTC_LOGGER(this, LOG_INFO, "wan start:1 generic opmode(%d) wmode(%d) ifs(%s)", mtc->rule.OMODE, mtc->rule.WMODE, mtc->rule.WAN);

    if(mtc->rule.OMODE == 6) {
       int probe;
       char probe_str[5];
       char *probeif[1];
 
       if(mtc->rule.WMODE == 9)
          probeif[0] = "br0";
       else
          probeif[0] = "eth0.0";

       exec_cmd2("ifconfig %s up", probeif);
       MTC_LOGGER(this, LOG_INFO, "wan start: start probeif(%s)", probeif[0]);
       exec_cmd3(probe_str, sizeof(probe_str), "smartprobe -I %s", probeif);
       probe = (int)probe_str;

       if(probe == 0) {
          MTC_LOGGER(this, LOG_INFO, "wan start: no server exists");
          if(mtc->rule.WMODE != 9) {
             MTC_LOGGER(this, LOG_INFO, "wan start: change to mobile 3G mode");
             cdb_set_int("$wan_mode", 9);
             cdb_set_int("$op_work_mode", 6);
             exec_cmd("/lib/wdk/commit");
             return 0;
          }
       }
       else {
          if(mtc->rule.WMODE == 9) {
             MTC_LOGGER(this, LOG_INFO, "wan start: change to non-mobile 3G mode");
             cdb_set_int("$wan_mode", probe);
             cdb_set_int("$op_work_mode", 6);
             exec_cmd("/lib/wdk/commit");
             return 0;
          }

          cdb_set_int("$wan_mode", probe);
          switch(probe) {
             case 2:
                 MTC_LOGGER(this, LOG_INFO, "wan start: start DHCP");
                 strcpy((char *)mtc->rule.WMODE, "2");
                 strcpy(mtc->rule.WAN, "eth0.0");
                 break;
                 
             case 3:
                 MTC_LOGGER(this, LOG_INFO, "wan start: start PPPoE");
                 strcpy((char *)mtc->rule.WMODE, "3");
                 strcpy(mtc->rule.WAN, "ppp0");
                 break;
                 
             default:
                 MTC_LOGGER(this, LOG_ERR, "wan start: error!, unknow probe option(%d)", probe);
                 return 0;
                 break;
          }
       }
       exec_cmd("sed -i -e 's;.*/etc/init.d/ethprobe$;;g' -e '/^$/d' /etc/crontabs/root");
       exec_cmd("*/1 * * * * . /etc/init.d/ethprobe > /etc/crontabs/root");
       exec_cmd("killall crond >dev/null 2>/dev/null");
       exec_cmd("/usr/sbin/crond restart -L /dev/null");
    }

    //MTC_LOGGER(this, LOG_DEBUG, "wan start:2 generic opmode(%d) wmode(%d) ifs(%s)", mtc->rule.OMODE, mtc->rule.WMODE, mtc->rule.WAN);

    if(mtc->rule.WMODE == 9) {
       exec_cmd3(str, sizeof(str), "`ifconfig|grep ppp0|sed 's/^ppp0.*$/ppp0/g'`");
       if(str == NULL)
          MTC_LOGGER(this, LOG_INFO, "wan start:ppp0 doesn't exist");
       else {
          MTC_LOGGER(this, LOG_INFO, "wan start:ppp0 already exist");
          return 0;
       }
    }

    cdb_set_int("$wanif_state", 1);
    if(cdb_set_int("$wan_arpbind_enable", 0) != 0)
       set_arp_wan_bind();

    exec_cmd("sed -i '//d' /etc/resolv.conf");

    switch(mtc->rule.WMODE) {
       case 1:
           static_ip_up();
           break;

       case 2:
           start_dhcp();
           break;
       
       case 3:
           MTC_LOGGER(this, LOG_INFO, "pppoe UP");
           save_pppoe_file();
           exec_cmd2("ifconfig %s up", mtc->rule.WANB);
           MTC_LOGGER(this, LOG_INFO, "ppp UP, wanb up");
           start_ppp();
           break;

       case 4:
           pptp_up();
           break;

       case 5:
           l2tp_up();
           break;

       case 9:
           if(cdb_get_str("$usbif_find_eth", usbif_find_eth, sizeof(usbif_find_eth), NULL) != NULL) {
              dialup();
              start_dhcp();
           }
           else {
              save_3gmodem_file();
              start_ppp();
           }
           break;
       
       default:
           MTC_LOGGER(this, LOG_INFO, "%s unknown up", mtc->rule.WAN);
           break;
    }
    return 0;
}

static int stop(void)
{
    char *ary[] = {"l2tpd", "dhcpcd", "pppd"};
    int ary_size;
    int i;
    clear_netstatus();

    MTC_LOGGER(this, LOG_ERR, "wan stop:");
    if((mtc->rule.IFPLUGD == NULL) && (mtc->rule.WMODE != 9))
       MTC_LOGGER(this, LOG_ERR, "wan stop fail: without ifplugd interface");
    else if(mtc->rule.WAN == NULL)
       MTC_LOGGER(this, LOG_ERR, "wan stop fail: null wan interface(%s)", mtc->rule.WAN);
    else {
       cdb_set("$wanif_state", "3");
       clear_arp_perm();

       exec_cmd("/usr/sbin/l2tp-control \"exit\" >/dev/null 2>&1");

       ary_size = (sizeof(ary))/(sizeof(ary[0]));
       for(i = 0; i < ary_size; i++) {
          exec_cmd2("killall -q -s 9 %s >/dev/null 2>&1", ary[i]);
       }

       exec_cmd2("ifconfig %s 0.0.0.0 down >/dev/null 2>&1", mtc->rule.WAN);
       exec_cmd2("ifconfig %s 0.0.0.0 down >/dev/null 2>&1", mtc->rule.WANB);

       MTC_LOGGER(this, LOG_INFO, "wan stop: generic opmode(%d) wmode(%d) ifs(%s)", mtc->rule.OMODE, mtc->rule.WMODE, mtc->rule.WAN);
    }
    cdb_set("$wanif_state", 0);
    return 0;
}

static int boot(void)
{
    /* ifplugd will call wan restart when interface is ready
     * excepting to 3g
     */
    if (mtc->rule.WMODE == wanMode_3g) {
        start();
    }

    return 0;
}

static montage_sub_proc_t sub_proc[] = {
    { "boot",  boot  },
    { "start", start },
    { "stop",  stop  },
    { .cmd = NULL    }
};

int wan_main(montage_proc_t *proc, char *cmd)
{
    MTC_INIT_SUB_PROC(proc);
    MTC_SUB_PROC(sub_proc, cmd);
    return 0;
}

