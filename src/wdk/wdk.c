#include "wdk.h"

int g_log_on;



int find_pid_by_name( char* proc_name, int* foundpid)
{
    DIR             *dir = NULL;
    struct dirent   *d = NULL;
    int             pid = 0;
    int i = 0;
    char  *s = NULL;
    int pnlen;

    foundpid[0] = 0;
    pnlen = strlen(proc_name);

    /* Open the /proc directory. */
    dir = opendir("/proc");
    if (!dir)   {
        LOG("find_pid_by_name: cannot open /proc");
        return -1;
    }

    /* Walk through the directory. */
    while ((d = readdir(dir)) != NULL) {

        char exe [PATH_MAX+1];
        char path[PATH_MAX+1];
        int len;
        int namelen;

        /* See if this is a process */
        if ((pid = atoi(d->d_name)) == 0)
            continue;

        snprintf(exe, sizeof(exe), "/proc/%s/exe", d->d_name);
        if ((len = readlink(exe, path, PATH_MAX)) < 0)
            continue;
        path[len] = '\0';

        /* Find ProcName */
        s = strrchr(path, '/');
        if(s == NULL)
            continue;
        s++;

        /* we don't need small name len */
        namelen = strlen(s);
        if(namelen < pnlen)     continue;

        if(!strncmp(proc_name, s, pnlen)) {
            /* to avoid subname like search proc tao but proc taolinke matched */
            if(s[pnlen] == ' ' || s[pnlen] == '\0') {
                foundpid[i] = pid;
                i++;
            }
        }
    }

    foundpid[i] = 0;
    closedir(dir);

    return  0;

}





/*
	Everytime read a line and print to the stdout
*/
void inline dump_file(char *file_path)
{
    FILE *fp = fopen(file_path, "r");
    char buf[200] = {0};
    while (fgets(buf, sizeof(buf), fp) != NULL ) {
        STDOUT("%s",  buf);
    }
}



int inline str_equal(char *str1, char *str2)
{
	return (strcmp(str1,str2) == 0);
}


void update_upload_state(int state)
{
	FILE *fp = NULL;
	char *remote_addr = NULL;
	char *file_id = NULL;

	remote_addr = getenv("REMOTE_ADDR");
	if (NULL == remote_addr)
		return;

	file_id = getenv("FILE_ID");
	if (NULL == file_id)
		return;

	if( (fp = fopen(WEB_UPLOAD_STATE, "a+")) ) {
		fprintf(fp, "%s %s %u %u\n", remote_addr, file_id, state, (unsigned int)time(NULL));
		fflush(fp);
		fsync(fileno(fp));
		fclose(fp);
		LOG("%s: state=%d ip=%s id=%s\n", __func__, state, remote_addr, file_id);
	}
}



struct wdk_program programs[] = {
	
	{
		.name = "sleep",
		.description = "sleep for a while.\n"
		"Usage:\n"
		"[sleep] sleep for 1 s\n"
		"[sleep X] sleep for X/1000 s",
		.function = wdk_sleep,
	},
	{
		.name = "ap",
		.description = "scan or get formatted scan_result\n"
		"Usage:\n"
		"[ap scan] scan for BSS"
		"[ap result] get pretty formatted ap scan result output",
		.function = wdk_ap,
	},
	{
		.name = "cdbreset",
		.description = "erase mtd1, you need to reboot after this operation",
		.function = wdk_cdbreset,

	},
	{
		.name = "debug",
		.description = "debug the command, print the cmd and print the output",
		.function = wdk_debug,

	},
	{
		.name = "dhcps",
		.description = "dhcp set/leases/num",
		.function = wdk_dhcps,

	},
	{
		.name = "get_channel",
		.description = "get current wifi channel",
		.function = wdk_get_channel,

	},
	{
		.name = "logout",
		.description = "logout from http??",
		.function = wdk_logout,

	},
	{
		.name = "ping",
		.description = "ping and get result",
		.function = wdk_ping,
	},
	{
		.name = "reboot",
		.description = "reboot linux system",
		.function = wdk_reboot,
	},
	{
		.name = "route",
		.description = "route table setting",
		.function = wdk_route,
	},
	{
		.name = "wan",
		.description = "connect or release wan connection",
		.function = wdk_wan,
	},
	{
		.name = "mangment",
		.description = "override httpd.conf user and password with cdb  value",
		.function = wdk_mangment,
	},

	{
		.name = "sta",
		.description = "dump current STA(s) connected to this AP",
		.function = wdk_sta,
	},

	{
		.name = "stor",
		.description = "delete or ls file in Disk Drive",
		.function = wdk_stor,
	},

	{
		.name = "stapoll",
		.description = "check current sta0 channel and apply to wlan0 by restarting service",
		.function =wdk_stapoll,
	},

	{
		.name = "status",
		.description = "get the status of wifi signal or battery power",
		.function =wdk_status,
	},

	{
		.name = "system",
		.description = "set  ntp,hostname,timezone and watchdog",
		.function =wdk_system,
	},


	{
		.name = "http",
		.description = "http",
		.function =wdk_http,
	},


	{
		.name = "sysupgrade",
		.description = "sysupgrade in /lib/wdk dir",
		.function =wdk_sysupgrade,
	},


	{
		.name = "wdkupgrade",
		.description = "sysupgrade in tmp dir ",
		.function =wdk_sysupgrade,
	},

	{
		.name = "time",
		.description = "get some information of time ",
		.function =wdk_time,
	},


	{
		.name = "upnp",
		.description = "get some information of miniupnpd lease",
		.function =wdk_upnp,
	},

	{
		.name = "cdbupload",
		.description = "get some information of miniupnpd lease",
		.function =wdk_cdbupload,
	},

	{
		.name = "log",
		.description = "send mailll",
		.function =wdk_log,
	},

	{
		.name = "logconf",
		.description = "start syslogd",
		.function =wdk_logconf,
	},

	{
		.name = "upload",
		.description = "query upload status",
		.function =wdk_upload,
	},

	{
		.name = "wps",
		.description = "wps 0/1 status/button/pin/cancel/genpin",
		.function = wdk_wps,

	},

	{
		.name = "smbc",
		.description = "upload local media files to samba server",
		.function = wdk_smbc,
	},

    {
        .name = "commit",
        .description = "cdb commit",
        .function = wdk_commit,
    },

    {
        .name = "save",
        .description = "cdb save",
        .function = wdk_save,
    },

    {
        .name = "rakey",
        .description = "wdk rakey",
        .function = wdk_rakey,
    },


};


int get_self_path(char *buf, int len)
{
    char szTmp[32] = {0};
    sprintf(szTmp, "/proc/%d/exe", getpid());
    int bytes = readlink(szTmp, buf, len);
    return bytes;
}

int readline2(char *base_path, char *ext_path, char* buffer, int buffer_size)
{
    FILE * fp = NULL;
    char path[200] = {0};
    strcat(path, base_path);
    strcat(path, ext_path);
    fp = fopen(path, "r");
    if (fp == NULL) {
        LOG("open file %s failed", path);
        return -1;
    }

    if((fgets(buffer, buffer_size, fp))!=NULL)   {
        fclose(fp);
        return 0;
    } else {
        LOG("fputs file %s failed", path);
        fclose(fp);
        return -1;
    }
}




int readline(char *path, char* buffer, int buffer_size)
{
	FILE * fp = NULL;
	fp = fopen(path, "r");
	if (fp == NULL) {
		LOG("open file %s failed", path);
		return -1;
	}

	if((fgets(buffer, buffer_size, fp))!=NULL)   {	
		fclose(fp);
		return 0;
	} else {
		LOG("fputs file %s failed", path);
		fclose(fp);
		return -1;
	}
}

int writeline(char *path, char* buffer)
{
    FILE * fp = NULL;
    fp = fopen(path, "w");
    if (fp == NULL) {
        LOG("open file %s failed", path);
        return -1;
    }
    if((fputs(buffer, fp)) >= 0)	 {
        fclose(fp);
        return 0;
    } else {
        LOG("fputs file %s failed", path);
        fclose(fp);
        return -1;
    }
}

int appendline(char *path, char* buffer)
{
    FILE * fp = NULL;
    fp = fopen(path, "a");
    if (fp == NULL) {
        LOG("open file %s failed", path);
        return -1;
    }
    if(fputs(buffer, fp) >= 0)	 {
        fclose(fp);
        return 0;
    } else {
        LOG("fputs file %s failed", path);
        fclose(fp);
        return -1;
    }
}


void enable_debug(int enable)
{
    if (enable) {
        putenv("WDK_DBG=1");
    } else {
        putenv("WDK_DBG=0");
    }
}


int check_debug()
{
	int enable = 0;
	
	char *value = getenv("WDK_DBG");
	if (NULL != value) {
		if (strcmp(value, "1") == 0)
			enable = 1;
	}
	return enable;
}


void debug_on()
{
	g_log_on = 1;
}


#if 0
/*
	check program exists in /lib/wdk dir
*/
int program_exists(char *program_name)
{
	DIR *dir = NULL; 
	struct dirent *dir_entry;
	int exist = 0;
	
	dir = opendir("/lib/wdk");
	if (NULL != dir) {
		while ((dir_entry = readdir(dir)) != NULL) {
			printf("\t%s\n", dir_entry->d_name );
			if (strcmp(dir_entry->d_name, program_name) == 0)
				exist = 1;
		}
		(void) closedir(dir);
	}

	return exist;	
}
#endif

void print_available_program()
{
	int i = 0;
	for (i = 0; i < sizeof(programs)/sizeof(programs[0]); i++) {
		printf("*  ");
		printf(programs[i].name);
		printf(":\n");
		printf(programs[i].description);
		printf("\n");
	}
}


void print_help()
{
	printf("\nHow to use this program:\n"
	"[/lib/wdk/program argument]:\n\t\trun wdk program with argument\n"
	"example:\n\t\t[/lib/wdk/ debug checkup] means run /lib/wdk/checkup with debug on\n"
	"How to turn on/off log:"
	"export WDK_DBG=1/export WDK_DBG=0"
	"\nAvailable programs:\n");

	print_available_program();
}


/*
	change to single file format
	
	example:
	/lib/wdk/commit -> commit
	./commit 		-> commit
	./wdk/commit 	-> commit

*/
char* get_real_name(char *full_name)
{
    int i = 0;
    for (i = strlen(full_name); i > 0 ; i--)
        if (full_name[i] == '/')
            return (full_name + i+1);
    return full_name;
}


static void printf_arguments(int argc, char **argv)
{
    int i = 0;
    char cmd_buffer[1000] = {0};
    strcat(cmd_buffer, "--------> wdk calling ");

    for (i = 0; i < argc; i++) {
        strcat(cmd_buffer, argv[i]);
        strcat(cmd_buffer, " ");

    }

    LOG("%s", cmd_buffer);
}


int main(int argc, char *argv[])
{
	int i = 0;
	int found = 0;
    struct timeval timecount;
	g_log_on = 0;


#if 0
	if (check_debug())
#endif	
		debug_on();

    printf_arguments(argc, argv);

    for (i = 0; i < sizeof(programs)/sizeof(programs[0]); i++) {
        if(strcmp(programs[i].name, get_real_name(argv[0])) == 0) {
            gettimeofbegin(&timecount);
            programs[i].function(argc - 1, argv + 1);
            LOG("[%s] spend=%ldms", programs[i].name, gettimevaldiff(&timecount));
            found = 1;
        }

    }

	if (0 == found) {
		printf("No such command:%s\n", argv[0]);
		print_help();

	}

    return 0;
}







