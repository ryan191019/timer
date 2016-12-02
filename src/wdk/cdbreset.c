#include "wdk.h"


int wdk_cdbreset(int argc, char **argv)
{
	if (argc != 0)
		return -1;

	if (remove("/etc/dnsmasq.conf") == 0) 
		LOG("remove dnsmasq.conf success"); 
	else 
		LOG("remove dnsmasq.conf fail"); 

	system("mtd erase mtd1");

	return 0;
}
