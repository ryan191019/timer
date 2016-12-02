

#include "wdk.h"




static int do_largepost()
{
	
	char alive_list[200] = {0};
	char kill_list[200] = {0};

	char *WDKUPGRADE_KEEP_ALIVE = NULL;
	if (f_exists("/tmp/wdkupgrade_before"))
		exec_cmd("/tmp/wdkupgrade_before");
	
	

	/*
		Get the list of program that we dont want to kill to free memory
		WDKUPGRADE_KEEP_ALIVE environment variable is also needed

		also we need to put http(myself) alive to drop cache
	*/
	
	strcat(alive_list, "init\\|uhttpd\\|hostapd\\|ash\\|watchdog\\|http\\|ps\\|$$");
	WDKUPGRADE_KEEP_ALIVE = getenv("WDKUPGRADE_KEEP_ALIVE");
	if (NULL != WDKUPGRADE_KEEP_ALIVE) {
		strcat(alive_list, "\\|");
		strcat(alive_list, WDKUPGRADE_KEEP_ALIVE);
	}
	LOG("Alive list = %s", alive_list);
	
	exec_cmd3(kill_list, sizeof(kill_list), "ps | grep -v '%s'  | awk '{if(NR > 2) print $1}' | tr '\n' ' ' | xargs kill -9", alive_list);
	
	/*
	root@router:/lib/wdk# free
              total         used         free       shared      buffers
	Mem:        28292        17500        10792            0         1276
	Swap:            0            0            0
	Total:        28292        17500        10792
	root@router:/lib/wdk# echo 3 > /proc/sys/vm/drop_caches
	root@router:/lib/wdk# free
				  total         used         free       shared      buffers
	Mem:        28292        14764        13528            0          312
	Swap:            0            0            0
	Total:        28292        14764        13528
	*/
	writeline("/proc/sys/vm/drop_caches", "3");
	sync();
	return 0;
}



int wdk_http(int argc, char **argv)
{
	if (argc != 1)
		return -1;

    if (strcmp(argv[0], "largepost") == 0) {
        do_largepost();
    } else if (strcmp(argv[0], "peerip") == 0) {
        /* echo peerip, the env value is from uhttpd */
        char *value = getenv("REMOTE_ADDR");
        if (NULL != value) {
            LOG("REMOTE IP=%s", value);
            STDOUT("%s\n", value);
        }
    } else if (strcmp(argv[0], "peermac") == 0) {
        char *value = getenv("REMOTE_ADDR");
        char mac[100] = {0};
        if (NULL != value) {
            LOG("REMOTE IP=%s", value);
            exec_cmd3(mac, sizeof(mac), "cat /proc/net/arp | grep \"%s\" | awk '{print $4}'", value);
            if (strlen(mac) > 0)
                STDOUT("%s\n", mac);
        }
    } else {
        return -1;
    }
    return 0;
}


