

#include "wdk.h"


struct iw_sta_info {

	char mac[20];
	char mode[20];
	char rate[20];
	char signal[20];

};


#if 0
static int count_substr(char *main_str, char *sub_str)
{
    int count = 0;
    char* tmp = main_str;

    while(*tmp != '\0' && (tmp = strstr(tmp, sub_str))) {
        ++count;
        ++tmp;
    }

    return count;
}
#endif


/*

root@uv:/lib/wdk# ./sta
oneline=Station bc:6c:21:73:f7:32 (on wlan0)
oneline=        inactive time:  170 ms
oneline=        rx bytes:       7561
oneline=        rx packets:     113
oneline=        signal:         -40 dBm
oneline=        tx bitrate:     72.2 MBit/s MCS 7 short GI
oneline=Station 88:53:95:40:f4:0e (on wlan0)
oneline=        inactive time:  5190 ms
oneline=        rx bytes:       24480
oneline=        rx packets:     222
oneline=        signal:         -34 dBm
oneline=        tx bitrate:     65.0 MBit/s MCS 6 short GI


*/
static void process_oneline(char *str, struct iw_sta_info *sta_info)
{
//	printf("oneline=%s\n", str);

    static int line_count = 0;
    char *tmp;

    switch(line_count) {
    case 0:
        memset(sta_info, 0, sizeof(struct iw_sta_info));
        str[25] = 0x00;
        strncpy(sta_info->mac, str + 8, sizeof(sta_info->mac));
        break;
    case 4:
        tmp = strstr(str, "dBm");
        *(tmp - 1) = 0x00;
        tmp = strstr(str, "-");
        strncpy(sta_info->signal , tmp, sizeof(sta_info->mac));
        break;
    case 5:
        if (strstr(str, "MCS") != NULL) {
            strncpy(sta_info->mode, "11n",sizeof(sta_info->mac));
            tmp = strstr(str, "MCS");
            *(tmp - 1) = 0;
            tmp = strstr(str, "bitrate");
            strncpy(sta_info->rate, tmp+9,sizeof(sta_info->mac));
        } else {
            strncpy(sta_info->mode, "11b/g", sizeof(sta_info->mac));
            tmp = strstr(str, "bitrate");
            strncpy(sta_info->rate, tmp+9,sizeof(sta_info->mac));
        }

        //print all
        STDOUT("mac=%s&mode=%s&rate=%s&signal=%s\n",
               sta_info->mac, sta_info->mode, sta_info->rate, sta_info->signal);

	break;
	default:
	break;
	}

	line_count++;
	if (6 == line_count)
		line_count = 0;
}

/*
	Dump result of [iw dev wlan0 station dump] and trasform the format.

	Note that the result may not accurate because there are station entry buffers
	still alive even sta get disconnected.
*/

int wdk_sta(int argc, char **argv)
{
    char buffer[500] = {0};
    char *temp;
    struct iw_sta_info sta_info;

    if (argc > 0) {
        return -1;
    }

    exec_cmd3(buffer, sizeof(buffer), "iw dev wlan0 station dump");
    if (strlen(buffer) == 0)
        return -1;

    temp = buffer;
    while(temp) {
        char * nextLine = strchr(temp, '\n');
        if (nextLine)
            *nextLine = '\0';	// temporarily terminate the current line
        process_oneline(temp, &sta_info);
        if (nextLine)
            *nextLine = '\n';	// then restore newline-char, just to be tidy
        temp = nextLine ? (nextLine+1) : NULL;
    }

	return 0;
}

