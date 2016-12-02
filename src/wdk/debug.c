
#include "wdk.h"


static int print_shell_result(char *command)
{
	FILE *fp = NULL;
	char result[100] = {0};
	fp = popen(command, "r");
	if (fp == NULL) {
		LOG("Failed to run command\n" );
		return -1;
	}

    while (fgets(result, sizeof(result), fp) != NULL) {
        STDOUT("%s", result);
    }
    pclose(fp);
    return 0;
}


int wdk_debug(int argc, char **argv)
{
    int i = 0;
    char cmd[100] = {0};
    for (i = 0; i < argc; i++) {
        strcat(cmd, argv[i]);
        strcat(cmd, " ");
    }
    STDOUT("cmd=%s\n", cmd);
    if (strlen(cmd) > 0)
        print_shell_result(cmd);

	return 0;
}

