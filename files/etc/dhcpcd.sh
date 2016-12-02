#!/bin/sh
#
#  This is a sample /etc/dhcpcd.sh script.
#  /etc/dhcpcd.sh script is executed by dhcpcd daemon
#  any time it configures or shuts down interface.
#  The following parameters are passed to dhcpcd.exe script:
#  $1 = HostInfoFilePath, e.g  "/var/lib/dhcpcd/dhcpcd-eth0.info"
#  $2 = "up" if interface has been configured with the same
#       IP address as before reboot;
#  $2 = "down" if interface has been shut down;
#  $2 = "new" if interface has been configured with new IP address;
#
#  Sanity checks

. /etc/cdb.sh
. /etc/rdb.sh
. /etc/if.sh


dhcpcd_up()
{
	config_get wanif_state wanif_state
	if [ "${wanif_state}" = "${STATE_CONNECTED}" ]; then
		return 0
	fi
# skip connected update for pptp and l2tp, if they are dhcp mode
	if [ ${WANMODE} -ne 4 -a ${WANMODE} -ne 5 ]; then
		config_set wanif_domain "${DNSDOMAIN}"
		config_set wanif_state ${STATE_CONNECTED}
	fi
	#/etc/init.d/wan_serv up
	mtc_cli wanipup
}

dhcpcd_down()
{
	#/etc/init.d/wan_serv down
	mtc_cli wanipdown
}

if [ $# -lt 2 ]; then
  logger -s -p local0.err -t dhcpcd.sh "wrong usage"
  exit 1
fi

hostinfo="$1"
state="$2"

# Reading HostInfo file for configuration parameters
. "${hostinfo}"

case "${state}" in
    up)
    logger -s -p local0.info -t dhcpcd.sh \
    "interface ${INTERFACE} has been configured with old IP=${IPADDR}"
    # Put your code here for when the interface has been brought up with an
    # old IP address here
    ;;

    new)
    logger -s -p local0.info -t dhcpcd.sh \
    "interface ${INTERFACE} has been configured with new IP=${IPADDR}"
    # Put your code here for when the interface has been brought up with a
    # new IP address

    dhcpcd_up
    ;;

    down) logger -s -p local0.info -t dhcpcd.sh \
    "interface ${INTERFACE} has been brought down"
    # Put your code here for the when the interface has been shut down

    dhcpcd_down
    ;;
esac
exit 0
