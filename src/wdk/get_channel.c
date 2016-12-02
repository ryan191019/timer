
#include "wdk.h"




/*
	get wifi channel and print
*/
int wdk_get_channel(int argc, char **argv)
{
    char result[100] = {0};
    if (argc != 0) {
        return -1;
    }

    /*
    	firstly get channel from sta0
    	because in wisp/repeater mode, wlan0 channel not accurate.
    */
    exec_cmd3(result, sizeof(result), "iw dev sta0 info | grep channel | awk '{print $2}'");
    if (strlen(result) > 0) {
        STDOUT("%d", atoi(result));
    } else {
        /* secondary get channel from wlan0 */
        exec_cmd3(result, sizeof(result), "iw dev wlan0 info | grep channel | awk '{print $2}'");
        if (strlen(result) > 0) {
            STDOUT("%d", atoi(result));
        } else {
            STDOUT("0");
        }
    }

	return 0;
}



