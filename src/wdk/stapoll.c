






#include "wdk.h"

#define STATUS_NOT_CONNECTED "Not connected"
#define STATUS_CONNECTED "Connected"

static int freq_to_channel(int freq)
{

	int channel = 0;
	
	switch (freq) {
	case 2412:
		channel = 1;
	break;
	case 2417:
		channel = 2;
	break;
	case 2422:
		channel = 3;
	break;
	case 2427:
		channel = 4;
	break;
	case 2432:
		channel = 5;
	break;
	case 2437:
		channel = 6;
	break;
	case 2442:
		channel = 7;
	break;
	case 2447:
		channel = 8;
	break;
	case 2452:
		channel = 9;
	break;
	case 2457:
		channel = 10;
	break;
	case 2462:
		channel = 11;
	break;
	case 2467:
		channel = 12;
	break;
	case 2472:
		channel = 13;
	break;
	case 2484:
		channel = 14;
	break;
	default:
		LOG("channel ERROR!\n");
		channel = 1;
	break;
	}

	return channel;
}


static int repair_channel()
{

	
	char tmp[50] = {0};
	int freq_cur = 0;
	int freq_cdb = 0;

	exec_cmd3(tmp, sizeof(tmp), "iw dev sta0 link |grep freq | awk '{print $2}'");
	if (strlen(tmp) == 0) {
		LOG("stapoll critical error 2\n");
		return -1;
	}
	freq_cur = freq_to_channel(atoi(tmp));
	freq_cdb = cdb_get_int("$wl_channel", 1);
	
	if (freq_cur != freq_cdb) {
		LOG("channel not equal, Cdb=%d Cur=%d\n", freq_cdb, freq_cur);
		cdb_set_int("$wl_channel", freq_cur);
		exec_cmd("/etc/init.d/wlan restart");
	} else {
		LOG("channel equal, Cdb=%d Cur=%d\n", freq_cdb, freq_cur);
	}

	return 0;
}




int wdk_stapoll(int argc, char **argv)
{

    char tmp[300] = {0};

    if (argc > 0) {
        return -1;
    }

	exec_cmd3(tmp, sizeof(tmp), "iw dev sta0 link");
	if (strlen(tmp) == 0) {
		LOG("stapoll critical error 1\n");
		return -1;
	}

	while (1) {
		if (strncmp(tmp, STATUS_CONNECTED, strlen(STATUS_CONNECTED)) == 0) {
			repair_channel();
			break;
		} else if (strncmp(tmp, STATUS_NOT_CONNECTED, strlen(STATUS_NOT_CONNECTED)) == 0) {
			LOG("Wait...");
			sleep(3);
			//continue to while here
		} else {
			LOG("stapoll critical error 3\n");
			return -1;
		}
	}

	return 0;
}



