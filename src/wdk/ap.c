#include "wdk.h"

#define PRINT_SCAN_PROCESS


int wdk_ap(int argc, char **argv)
{
    if (argc < 1 || argc > 2)
        return -1;

    if (str_equal(argv[0], "scan")) {
        exec_cmd("ifconfig sta0 up");
#ifdef PRINT_SCAN_PROCESS
        system("iw dev sta0 scan");
#else
        exec_cmd("iw dev sta0 scan");
#endif
    } else if (str_equal(argv[0], "result")) {
        /*
        	Bug fixed in R7610, need to cd to / before
        */
        exec_cmd("cd /;echo scan_results > /proc/wd;cat /proc/wd");
        dump_file("/etc/res");
        exec_cmd("rm /etc/res");
    }

	return 0;
}





