

#include "wdk.h"



int wdk_logconf(int argc, char **argv)
{
	char log_sys_ip[100] = {0};
	int log_rm_enable = 0;
	
	exec_cmd("killall syslogd");

	log_rm_enable = cdb_get_int("$log_rm_enable", 0);
	cdb_get("$log_sys_ip", log_sys_ip);
	if (f_exists( "/sbin/syslogd")) {
		if (1 == log_rm_enable) {
			LOG("Starting syslog to %s", log_sys_ip);
			exec_cmd2("syslogd -s 1 -L -R %s:514", log_sys_ip);
		}
	}

	return 0;
}




