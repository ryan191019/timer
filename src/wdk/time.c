
#include "wdk.h"




/*
	argument = 0:
		`cat /proc/uptime`
	argument = 1:
		ntp:`date +%s`				-> date in second format
		ntpstr:`date +%c`			-> date in readable string format
		sync:`date -s yourtime`		-> set date string to system
		any: report error
	

*/
int wdk_time(int argc, char **argv)
{

    char cmd_buffer[100] = {0};
    char date_set_str[100] = {0};
    int i = 0;
    if (argc == 0) {
        readline("/proc/uptime", cmd_buffer, sizeof(cmd_buffer));
        while (cmd_buffer[i++] != '.')
            ;
        cmd_buffer[i-1] = 0x00;
        STDOUT("%s\n", cmd_buffer);
    } else if (argc >= 1) {
        if (strcmp(argv[0], "ntp") == 0) {
            exec_cmd3(cmd_buffer, sizeof(cmd_buffer), "date +%%s");
            STDOUT("%s\n", cmd_buffer);
        } else if (strcmp(argv[0], "ntpstr") == 0) {
            exec_cmd3(cmd_buffer, sizeof(cmd_buffer), "date +%%c");
            STDOUT("%s\n", cmd_buffer);
        } else if (strcmp(argv[0], "sync") == 0) {
            exec_cmd3(date_set_str, sizeof(date_set_str), "echo '' | awk '{print strftime(\"%%F %%T\", %s)}'", argv[1]);
            STDOUT("%s\n", date_set_str);
            exec_cmd2("date -s \"%s\"", date_set_str);
        } else {
            LOG("Please input right paramenter:[ntpstr/ntp/(empty)]");
            return -1;
        }
    }

	return 0;
}

