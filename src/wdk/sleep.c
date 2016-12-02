
#include "wdk.h"


int wdk_sleep(int argc, char **argv)
{
	if (argc == 0) {
		LOG("sleep for 1000ms");
		sleep(1);
		return 0;
	} else if(argc == 1) {
		LOG("Sleep for %ds", atoi(argv[0])/1000);
		sleep(atoi(argv[0])/1000);
		return 0;
	} else {
		return -1;
	}
}

