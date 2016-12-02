
#include "wdk.h"




// logout.htm send _.cliCmd("logout") -> /lib/wdk/logout
int wdk_logout(int argc, char **argv)
{
	if (argc != 0) {
			return -1;
	}

	kill_pidfile("/tmp/httpd.pid");
	
	return 0;
}
