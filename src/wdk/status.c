



#include "wdk.h"

#define WORK_MODE_WISP 3


static void get_wifi_signal()
{
	int op_work_mode = -1;
	char tmp[50] = {0};

    op_work_mode = cdb_get_int("$op_work_mode", 0);
    if (WORK_MODE_WISP == op_work_mode) {
        exec_cmd3(tmp, sizeof(tmp), "iw dev sta0 link | grep signal | awk '{print $2}'");
        STDOUT("%s\n", tmp);
    } else {
        STDOUT("\n");
    }
}


int wdk_status(int argc, char **argv)
{
	if (argc != 1) {
		return -1;
	}

	if (strcmp(argv[0], "wifi_signal") == 0) {
		get_wifi_signal();
	} else if (strcmp(argv[0], "batt_power") == 0) {
		printf("100\n");
	}

	return 0;
}













