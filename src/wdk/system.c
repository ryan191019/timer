


#include "wdk.h"


static void do_ntp()
{
	char sys_ntp_svr1[100] = {0};
	int sys_ntp_update = 0;
	int sys_ntp_retry = 0;

	cdb_get("$sys_ntp_svr1", sys_ntp_svr1);
	sys_ntp_update = cdb_get_int("$sys_ntp_update", 0);
	sys_ntp_retry = cdb_get_int("$sys_ntp_retry", 0);


	if (0 == sys_ntp_retry)
		sys_ntp_retry = 3;

	if (0 == sys_ntp_update)
		sys_ntp_update = 86400;

	if (strlen(sys_ntp_svr1) == 0) {
		strcpy(sys_ntp_svr1, "0.asia.pool.ntp.org");
	}

	exec_cmd("killall -q -s 9 ntpclient");
	exec_cmd2("start-stop-daemon -S -x /usr/sbin/ntpclient -b -- -l -h %s -c %d -s -i %d",
			sys_ntp_svr1, sys_ntp_retry, sys_ntp_update);
}


static void do_hostname()
{

	char sys_name[100] = {0};
	char hostname[100] = {0};
	char lan_ip[50] = {0};
	char lan_host[50] = {0};


	cdb_get("$sys_name", sys_name);

	exec_cmd3(hostname, sizeof(hostname), "cat /proc/sys/kernel/hostname");
	if (strcmp(hostname, sys_name) != 0) {
		LOG("Setting cdb sys_name to kernel hostname");
		exec_cmd2("echo -n  \"%s\" >  /proc/sys/kernel/hostname", sys_name);
		if (strcmp(hostname, "(none)") != 0) {
			/* it is not in boot process now
 			hostname is changed, need to update /etc/hosts
			*/
			LOG("Setting /etc/hosts");
			writeline("/etc/hosts", "127.0.0.1 localhost\n");

			cdb_get("$lan_ip", lan_ip);
			if ((strlen(lan_ip) > 0) && (strlen(sys_name) > 0)) {
				sprintf(lan_host, "%s %s\n", lan_ip, sys_name);
				LOG("Appending: %s", lan_host);
				appendline("/etc/hosts", lan_host);
			}
		}
	}

}


static void do_timezone()
{

	char sys_day_info[20] = {0};
	int timezone = 0;
	char timezone_grep[200] = {0};
	char timezone_grep_result[200] = {0};
	char *p0 = NULL;
	char *p1 = NULL;


	if (0 != access("/lib/data/timezones.txt", 0)) { 
		return;
	}

	cdb_get("$sys_day_info", sys_day_info);
	if (strlen(sys_day_info) == 0)
		return;

	if (strcmp(sys_day_info, "!ERR") == 0)
		return;

    sscanf(sys_day_info, "tz=%d", &timezone);
    //LOG("timezone=%d", timezone);
    sprintf(timezone_grep, "cat /lib/data/timezones.txt | grep '\"%d\"'", timezone);
    exec_cmd3(timezone_grep_result, sizeof(timezone_grep_result), timezone_grep);

    //LOG("Timezone line=%s", timezone_grep_result);

	p0 = strstr(timezone_grep_result, ",\"");
	p0 = p0+2;
	p1 = p0;

	while (1) {
		p1++;
		if ((*p1 == '"') ||(*p1 == ',')  )
			break;
	}
	*p1 = '\n';
	*(p1+1) = 0x00;

    writeline("/etc/TZ", p0);

}

static void  do_watchdog()
{

	int sys_wd_en = -1;
	int sys_wd_idle = -1;

	sys_wd_en = cdb_get_int("$sys_wd_en", 0);
	sys_wd_idle = cdb_get_int("$sys_wd_idle", 0);

	LOG("kill watchdog");
	exec_cmd("killall watchdog");
	if (1 == sys_wd_en) {
		LOG("Starting watchdog with idle=%d", sys_wd_idle);
		exec_cmd2("watchdog -t 5 -T %d /dev/watchdog", sys_wd_idle);
	}
}

static void do_default()
{
	do_hostname();
	do_timezone();
	do_watchdog();
}



int wdk_system(int argc, char **argv)
{
    if (argc == 0) {
        do_default();
    } else if(argc == 1) {
        //LOG("Argument is %s", argv[0]);
        if(strcmp(argv[0], "ntp") == 0) {

            do_ntp();
        } else if(strcmp(argv[0], "hostname") == 0) {
            do_hostname();
        } else if(strcmp(argv[0], "timezone") == 0) {
            do_timezone();
        } else if(strcmp(argv[0], "watchdog") == 0) {
            do_watchdog();
        } else  {
            LOG("Unknown command");
            return -1;
        }
    } else {
        LOG("Unknown command");
        return -1;
    }

	return 0;
}




