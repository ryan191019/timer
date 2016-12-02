

#include "wdk.h"


int wdk_ping(int argc, char **argv)
{
    char outbuf[100] = {0};
    if (argc != 1) {
        return -1;
    }

    exec_cmd3(outbuf, sizeof(outbuf), "/bin/ping %s -c 1 | sed -n '/seq/s/^.*seq/seq/p'", argv[0]);
    STDOUT("%s\n", outbuf);

	return 0;
}



