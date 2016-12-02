#include "wdk.h"


int wdk_wan(int argc, char **argv)
{
    if (argc == 0) {
        LOG("usage:/lib/wdk/wan [connect/release]\n");
        return 0;
    } else if(argc == 1) {

		putenv("manual_onoff=1");

		if (strcmp("connect", argv[0]) == 0) {
			exec_cmd("/etc/init.d/wan stop");
			exec_cmd("/etc/init.d/wan start");
		} else if (strcmp("release", argv[0]) == 0)  {
			exec_cmd("/etc/init.d/wan stop");
		} else {
			return -1;
		}
		return 0;
	} else {
		return -1;
	}
}





