
#define _GNU_SOURCE
#include "wdk.h"
#include <sys/reboot.h>

#define SYS_UP_FLASH_EXE "/sbin/sysupgrade"
#define SYS_UP_RAM_EXE "/tmp/sysupgrade"



static void free_memory(int all)
{
	char alive_list[200] = {0};
	char kill_list[200] = {0};
	char *WDKUPGRADE_KEEP_ALIVE = NULL;


	if (all)
		strcat(alive_list, "init\\|ash\\|watchdog\\|wdkupgrade\\|ps\\|$$");
	else
		strcat(alive_list, "init\\|uhttpd\\|hostapd\\|ash\\|watchdog\\|wdkupgrade\\|http\\|ps\\|$$");

	WDKUPGRADE_KEEP_ALIVE = getenv("WDKUPGRADE_KEEP_ALIVE");

	if (NULL != WDKUPGRADE_KEEP_ALIVE) {
		strcat(alive_list, "\\|");
		strcat(alive_list, WDKUPGRADE_KEEP_ALIVE);
	}

	//LOG("Kill programs to free memory without: %s", alive_list);
	LOG("Free memory now, all=%d", all);
	exec_cmd3(kill_list, sizeof(kill_list), "ps | grep -v '%s'	| awk '{if(NR > 2) print $1}' | tr '\n' ' ' | xargs kill -9", alive_list);
	writeline("/proc/sys/vm/drop_caches", "3");
	sync();
	return;

}


/*
	Get the flash firmware size of current board
*/
static int get_flash_size()
{
	char flash_size[100] = {0};
	exec_cmd3(flash_size, sizeof(flash_size), "cat /proc/flash  | grep \"firmware\" | awk '{print $1}'");

	if (strlen(flash_size) > 0)
		return atoi(flash_size);
	else
		return 0;
}



/*
	Get the flash image to be uploaded
*/
static int cal_image_size()
{
	char flash_size[100] = {0};
	exec_cmd3(flash_size, sizeof(flash_size), "wc -c /tmp/firmware | awk '{print $1}'");

	if (strlen(flash_size) > 0)
		return atoi(flash_size);
	else
		return 0;
}


/*

	calculate the backup playlist file size(sum up)

*/
static int cal_backup_size()
{


	//TODO


#if 0
	local sizes=`cat $ {backup_list} 2> /dev/null | while read file;
do
	[ -d $file ] && {
		    du -s $ {file} | awk '{print $1*1024}'
		} || {
		wc -c ${file} | awk '{print $1}'
	}
	done`
	echo "${sizes}" | awk '{sum += $1} END{print sum}'
#endif

	return 0;
}



#if 0
/*
	Check the firmware versions of ([cdb version] and [firmware.img version]) are same.

	same 		=> return 1
	not same	=> return 0

*/
static int check_firmware_version()
{
	char sw_ver[100] = {0};
	char fw_ver[100] = {0};

	cdb_get("$sw_ver", sw_ver);

	exec_cmd3(fw_ver, sizeof(fw_ver), "hexdump -s 24 -n 4 -e '\"%%d\"' %s", "/tmp/firmware" );
	if (atoi(fw_ver) == atoi(sw_ver)) {
		LOG("firmware versions are same %s, quit update", fw_ver);
		return 1;
	} else {
		LOG("firmware versions not same %s != %s, meet the need", fw_ver, sw_ver);
		return 0;
	}

}
#endif

static int check_flash_size()
{
	int flashSize = get_flash_size();
	int imageSize = cal_image_size();
	int backupSize = cal_backup_size();
	int leftSize = 0;

	if (0 == flashSize)
		flashSize = 8388608;

	leftSize = flashSize - imageSize - backupSize;

	LOG(">>>>flashSize = %dByte", flashSize);
	LOG(">>>>imageSize = %dByte", imageSize);
	LOG(">>>>backupSize = %dByte", backupSize);
	LOG(">>>>leftSize = %dByte", leftSize);


	if ((leftSize > 0) ||(imageSize <= 0)) {
		return 1;
	} else {
		return 0;
	}
}


static int check_battery_status()
{
	/* Todo implemented */
	LOG("No battery, pass");
	return 1;
}


static int check_powerline_status()
{
	/* Todo implemented */
	LOG("No powerline, pass");
	return 1;
}



static int check_checksum(char *firmware_path)
{
	char chksum[100] = {0};
	char hwid[100] = {0};

	exec_cmd3(hwid, sizeof(hwid), "cat /proc/bootvars | grep id= | cut -d '=' -f 2" );
	exec_cmd3(chksum, sizeof(chksum), "chksum -i %s -h %s  -v 1;echo $?", firmware_path, hwid);
	if (atoi(chksum) == 1) {
		LOG("Firmware File chksum failed!");
		return 1;
	} else {
		LOG("Firmware File chksum passed!");
		return 0;
	}
}



/*

	Check if now is suitable for firmware upgrade,
	If ok, return 0
	If fail, return 1
*/
static int check_firmware_upgrade()
{

	LOG("=======>check_firmware_upgrade for conditions");
	if (check_checksum("/tmp/firmware") == 1)
		return 1;

#if 0
	if (check_firmware_version() == 1) {
		LOG("Cancel upgrade, Firmware Version the same!");
		return 1;
	}
#endif

	if  (check_flash_size() == 0) {
		LOG("Cancel upgrade, Flash Space not enough!!");
		return 1;
	}

	if (check_battery_status() == 0) {
		LOG("Cancel upgrade, Low Battery!!");
		return 1;
	}

	if (check_powerline_status() == 0) {
		LOG("Cancel upgrade, No Power Line!!");
		return 1;
	}

	LOG("=======>check_firmware_upgrade done");

	return 0;

}



static void reboot_directly()
{
	LOG("Reboot Directly!!");
	exec_cmd("/sbin/reboot2");
	sleep(1);
	LOG("Oops!! Reboot Again!!");
	exec_cmd("/sbin/reboot2");
	reboot(RB_AUTOBOOT);
}

static void disable_watchdog()
{
	LOG( "kill watchdog!");
	exec_cmd("killall watchdog");
}



static void before_upgrade()
{

	if (f_exists("/tmp/wdkupgrade_before"))
		exec_cmd("/tmp/wdkupgrade_before");

	free_memory(1);

	if (f_isdir("/overlay/playlists)"))  {
		writeline("/tmp/backup.lst", "/overlay/playlists)");
	}

	exec_cmd("cdb revert");
	cdb_commit();
}

static void after_upgrade()
{

	if (f_exists("/tmp/wdkupgrade_after")) {
		exec_cmd("/tmp/wdkupgrade_after");
	}
	
	disable_watchdog();
	LOG("after upgrade: done");
}


/*
	Firmware header bytes: 0x41 0x49 0x48 ===> (AIH)

*/
static int check_firmware_header(char *path)
{
	FILE *fw_fp;
	unsigned char k[3] = {0};
	fw_fp=fopen(path,"r");
	fread(k,3,1,fw_fp);
	fclose(fw_fp);


	if ((k[0] == 0x41) && (k[1] == 0x49) &&
		(k[2] == 0x48))
		return 1;
	else
		return -1;

}

/*

	Check the firmware of offset byte 22,

	unsigned short flags; 
	#define IH_EXECUTABLE (1<<4)


	0x10 -> it is bootexe
	else -> it is not bootexe
*/
static int check_boot_exe(char *path)
{
    FILE *fw_fp = NULL;
    unsigned char k[22] = {0};
    fw_fp=fopen(path,"r");
    if (NULL == fw_fp)
        return -1;
    fread(k,22,1,fw_fp);
    fclose(fw_fp);


	if (k[21] &0x10)
		return 1;
	else
		return -1;
}


static void upgrade_core()
{

	char update_result[100] = {0};

	/*
	 *	If the firmware is bootexe, run the firmware as elf program
	 *	else the firmware is just the firmware
	 */
	if (1 == check_boot_exe("/tmp/firmware")) {
		//bootexe means it is the exe that can update the boot,just write the boot.img to mtd0
		LOG("Updating boot with bootexe..");
		exec_cmd("dd if=/tmp/firmware of=/tmp/firmware.tmp bs=1 skip=48; chmod +x /tmp/firmware.tmp; /tmp/firmware.tmp");
	} else {
		LOG("Executing /sbin/sysupgrade");
		exec_cmd3(update_result, sizeof(update_result), "/sbin/sysupgrade /tmp/firmware;echo $?");
	}

	LOG("Update result=%s", update_result);

}

static void start_upgrade(char *dir, char *firmware_name)
{
 	char copy_result[10] = {0};
	char fw_path[100] = {0};
	strcat(fw_path, dir);
	strcat(fw_path, "/");
	strcat(fw_path, firmware_name);

	LOG("Open firmware %s to check the header", fw_path);
	if (-1 == (check_firmware_header(fw_path))) {
		LOG("firmware header check failed");
		return;
	} else {
		LOG("firmware header check passed");
	}

	free_memory(0);

	LOG("Copy file from %s to /tmp/firmware", fw_path);
	exec_cmd3(copy_result, sizeof(copy_result) , "cp %s /tmp/firmware;echo $?", fw_path);

	if (strcmp(copy_result, "0") != 0) {
		LOG("Copy file failed");
		goto update_end;
	}


	before_upgrade();
	if (1 == check_firmware_upgrade()) {
		LOG("Firmware Upgrade failed");
		goto update_end;
	}

	upgrade_core();

update_end:
	after_upgrade();        
	sleep(3);       
	reboot_directly();

}


static void scan_usb_mmc()
{

	FILE *stream;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	char sys_auto_upgrade_name[100] = {0};
	cdb_get("$sys_auto_upgrade_name", sys_auto_upgrade_name);


	LOG("Read file content of /var/diskinfo.txt");
	stream = fopen("/var/diskinfo.txt", "r");
	if (stream == NULL) {
		LOG("Result shows no disk inserted");
		return;
	}

	while ((read = getline(&line, &len, stream)) != -1) {
		line[strlen(line) -1] = 0x00; /* delete the \n, or opendir will fail */
		LOG("Found storage device:%s, start to search for %s", line, sys_auto_upgrade_name);
		
		DIR *d = NULL;
		struct dirent *dir = NULL;
		d = opendir(line);
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				if (dir->d_name[0] == '.')
					continue;
				if ((dir->d_type == DT_REG) && (strcmp(dir->d_name, sys_auto_upgrade_name) == 0)) {
					LOG("Found firmware:%s/%s, start to upgrade", line, dir->d_name);
					start_upgrade(line, sys_auto_upgrade_name);
				}
			}
			closedir(d);
		} else {
			LOG("opendir %s  failed, err= %d", line, errno);
		}
	}
	
	LOG("Search End");
	free(line);
	fclose(stream);

}

#ifdef CONFIG_FIELD_TESTING
//#if 1
static void start_field_test(char *name)
{	
	FILE *stream;
	char *line = NULL;
	char *ssid = NULL;
	char *password = NULL;
	ssize_t read;
	size_t len = 0;
	
	stream = fopen(name, "r"); 
	if (stream == NULL) {
			LOG("open %s faield",name);
			return;
	}

	while ((read = getline(&line, &len, stream)) != -1) {
		line[strlen(line) -1] = 0x00; /* delete the \n, or opendir will fail */
		LOG("line:%s", line);
		char *tmp = NULL ;
		if ((tmp=strstr(line, "ssid=")) != NULL) {
			
			ssid = strdup(tmp);
			LOG("ssid:%s", ssid);
		} else if ((tmp=strstr(line, "password=")) != NULL ) {
			
			password = strdup(tmp);
			LOG("password:%s", password);
		} else {
			continue;
		}
	}

	exec_cmd("aplay /media/sda*/usb.wav");
	exec_cmd("aplay /media/mmcblk*/sd.wav");
	if(ssid == NULL || password == NULL) {
		exec_cmd("elian.sh connect DLNA 12345678");
		
	} else {
		char cmd[128];
		snprintf(cmd, 128,"elian.sh connect \"%s\" \"%s\"", ssid, password);
		LOG("cmd:%s", cmd);
		exec_cmd(cmd);
	}
	
	
	free(line);
	fclose(stream);
	
}

static void scan_field_config()
{

	FILE *stream;
	char *line = NULL;
	size_t len = 0;
	ssize_t read;

	//char config_name[100] = {0};
	char *config_name = "field-testing.config";
	//cdb_get("$sys_field_testing_config_name", sys_auto_upgrade_name);


	LOG("Read file content of /var/diskinfo.txt");
	stream = fopen("/var/diskinfo.txt", "r");
	if (stream == NULL) {
		LOG("Result shows no disk inserted");
		return;
	}

	while ((read = getline(&line, &len, stream)) != -1) {
		line[strlen(line) -1] = 0x00; /* delete the \n, or opendir will fail */
		LOG("Found storage device:%s, start to search for %s", line, config_name);
		
		DIR *d = NULL;
		struct dirent *dir = NULL;
		d = opendir(line);
		if (d) {
			while ((dir = readdir(d)) != NULL) {
				if (dir->d_name[0] == '.')
					continue;
				if ((dir->d_type == DT_REG) && (strcmp(dir->d_name, config_name) == 0)) {
					LOG("Found config:%s/%s, start to field testing...", line, dir->d_name);
					char name[64]={0};
					snprintf(name, 64, "%s/%s",line, dir->d_name);
					start_field_test(name);
					//start_upgrade(line, sys_auto_upgrade_name);
					
				}
			}
			closedir(d);
		} else {
			LOG("opendir %s  failed, err= %d", line, errno);
		}
	}
	
	LOG("Search End");
	free(line);
	fclose(stream);
}



#endif

/*

	Read diskfile and auto upgrade firmware from disk when disk is inserted to board

	1.The name must be $sys_auto_upgrade_name, firmware.img default
	2.Firmware version must be not same, defined in arch/mips/boot/Makefile
	3.Battery power low will cause fail and exit

*/
static void auto_firmware_upgrade(char *para)
{
	int sys_auto_upgrade = 0;

	sys_auto_upgrade = cdb_get_int("$sys_auto_upgrade", 0);

	if (0 == sys_auto_upgrade) {
		LOG("cdb checked sys_auto_upgrade is 0, auto upgrade firmware exit");
		return;
	}

	LOG("Scanning disk for firmware upgrade");

	/*
	root@router:/# cat /var/diskinfo.txt
	/media/sda1
	/media/sdb1
	*/

	scan_usb_mmc();
#ifdef CONFIG_FIELD_TESTING
	scan_field_config();
#endif


}






static void web_firmware_upgrade()
{

	int firmware_ready = -1;
	update_upload_state(WEB_STATE_ONGOING);
	free_memory(0);
	firmware_ready = check_firmware_upgrade();

	if (firmware_ready != 0) {
		update_upload_state(WEB_STATE_FAILURE);
		LOG("check failed, Do nothing");
		goto update_end;
	} else {
		update_upload_state(WEB_STATE_FWSTART);
		sleep(3);
		before_upgrade();

		LOG("Writing firmware_file to flash...");
		upgrade_core();
	}

	//TODO Judge firmware update fail via update_result
	update_upload_state(WEB_STATE_SUCCESS);

update_end:
	after_upgrade();
	sleep(3);
	reboot_directly();

}


/*
If you want to do something such as light on a led before sysupgrade or after,
please put scripts with the name: 
/lib/wdk/sysupgrade_before
/lib/wdk/sysupgrade_after


*/
int wdk_sysupgrade(int argc, char **argv)
{

	char path[1024] = {0};
	char daemon_cmd[1024] = {0};
	int i = 0;

	get_self_path(path, sizeof(path));

	LOG("sysupgrade is curently at %s", path);
	if (strcmp("/lib/wdk/wdk-bin", path) == 0) {
		exec_cmd("cp /lib/wdk/sysupgrade /tmp/wdkupgrade");

		if (f_exists("/lib/wdk/sysupgrade_before")) {
			exec_cmd("cp /lib/wdk/sysupgrade_before /tmp/wdkupgrade_before");
			exec_cmd("chmod +x /tmp/wdkupgrade_before");
		}
		
		if (f_exists("/lib/wdk/sysupgrade_after")) {
			exec_cmd("cp /lib/wdk/sysupgrade_after /tmp/wdkupgrade_after");
			exec_cmd("chmod +x /tmp/wdkupgrade_after");
		}
		
		for (i = 0; i < argc; i++) {
			strcat(daemon_cmd, argv[i]);
			strcat(daemon_cmd, " ");
		}
		//-S:start a process unless a matching process is found.
		LOG("Pass daemon cmd = %s", daemon_cmd);
		exec_cmd2("start-stop-daemon -S -N -10 -x /tmp/wdkupgrade -b -- %s", daemon_cmd);
	} else if  (strcmp("/tmp/wdkupgrade", path) == 0) {
		if ((argc > 1) && strcmp(argv[0], "auto") == 0) {
			if (argc >= 2) {
				LOG("Enter auto_firmware_upgrade, argument = %s", argv[1]);
				auto_firmware_upgrade(argv[1]);
			} else {
				LOG("Enter auto_firmware_upgrade");
				auto_firmware_upgrade(NULL);
			}
		} else {
			LOG("Enter web_firmware_upgrade");
			web_firmware_upgrade();
		}
	}

	return 0;

}



