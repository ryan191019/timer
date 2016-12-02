


#include "wdk.h"



/*

From miniupnpd package lease file

===>lease_file_add
fprintf( fd, "%s:%hu:%s:%hu:%s\n",
((proto==IPPROTO_TCP)?"TCP":"UDP"), eport, iaddr, iport, desc);


root@(none):/lib/wdk# cat /var/lease 
TCP:12345:192.168.2.2:222:helloworld
root@(none):/lib/wdk# ./upnp map       
gp=12345&lip=192.168.2.2&lp=222&prot=TCP&desc=helloworld



*/


static void show_lease()
{
	char lease_line[200] = {0};
	char proto[100] = {0};
	char iaddr[100] = {0};
	char eport[100] = {0};
	char iport[100] = {0};
	char desc[100] = {0};

	char *p = NULL;
	readline("/var/lease", lease_line, sizeof(lease_line));
	p = lease_line + strlen(lease_line) -1;

	while (*p-- != ':');
	*(p+1) = 0x00;
	strcpy(desc, p + 2);

	
	while (*p-- != ':');
	*(p+1) = 0x00;
	strcpy(iport, p + 2);

	while (*p-- != ':');
	*(p+1) = 0x00;
	strcpy(iaddr, p + 2);

	while (*p-- != ':');
	*(p+1) = 0x00;
	strcpy(eport, p + 2);

	strcpy(proto, lease_line);

    STDOUT("gp=%s&lip=%s&lp=%s&prot=%s&desc=%s", eport, iaddr, iport, proto, desc);

}

int wdk_upnp(int argc, char **argv)
{
	if (argc == 0)
		return -1;

	if (strcmp(argv[0], "map") == 0) {
		show_lease();
	}

	return 0;
}


