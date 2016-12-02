
#include <sys/reboot.h>
#include "wdk.h"



int wdk_reboot(int argc, char **argv)
{

	int pidof_uhttpd[128] = {0};  
	if (argc != 0) {
		return -1;
	}


	LOG("System is rebooting...");

	exec_cmd("/etc/init.d/wlan stop");
	sleep(3);
	if (0 == find_pid_by_name("uhttpd", pidof_uhttpd)) {
		LOG("killing uhttpd pid=%d", pidof_uhttpd[0]);
		kill(pidof_uhttpd[0], SIGKILL); /*assume only one uhttpd appears*/
	}

	reboot(RB_AUTOBOOT);

	return 0;
}






