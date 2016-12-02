


#include "wdk.h"

/*
	example output:

	FORMAT: IP ? STATE TIME

	root@router:/# cat /tmp/upload_state 
	192.168.1.213 0 1 1479180147
	192.168.1.213 0 1 1479180147
	192.168.1.213 0 2 1479180148

	get the last line from /tmp/upload_state 
	with the information of ${REMOTE_ADDR} ${1}

	If success or fail ,delete the related lines of ${REMOTE_ADDR} ${1}

*/
int wdk_upload(int argc, char **argv)
{

	char *REMOTE_ADDR = NULL;
	char upload_state_select[100] = {0};
	char current_state[10] = {0};

	if (argc != 2)
		return -1;

	if (!str_equal(argv[0], "state"))
		return -1;

	REMOTE_ADDR = getenv("REMOTE_ADDR");
	if (NULL == REMOTE_ADDR)
		return -1;

	sprintf(upload_state_select, "%s %s", REMOTE_ADDR, argv[1]);

	exec_cmd3(current_state, sizeof(current_state),
	"cat /tmp/upload_state  | grep \"%s\" | awk '{print $3}' | tail -n 1",
	upload_state_select);

	LOG("upload_state_select = %s, current_state= %s", upload_state_select, current_state);

	/* clear all if done*/
	if ((atoi(current_state) == WEB_STATE_SUCCESS) || 
		(atoi(current_state) == WEB_STATE_FAILURE)) {
		exec_cmd2("sed -i \"/%s /d\" /tmp/upload_state", upload_state_select);
	}

    if (strlen(current_state) > 0)
        STDOUT("%s", current_state);
    else
        STDOUT("%d", WEB_STATE_DEFAULT);

	return 0;

}




