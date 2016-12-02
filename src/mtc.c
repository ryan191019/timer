
/**
 * @file mtc.c
 *
 * @author Frank Wang
 * @date   2015-10-06
 */

#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <ctype.h>

#include "include/mtc.h"
#include "include/mtc_ipc.h"
#include "include/mtc_linklist.h"
#include "include/mtc_time.h"
#include "include/mtc_misc.h"
#include "include/mtc_inf.h"
#include "include/mtc_fsm.h"
#include "include/mtc_proc.h"



#define cprintf(fmt, args...) do { \
        FILE *fp = fopen("/dev/console", "w"); \
        if (fp) { \
                fprintf(fp, fmt , ## args); \
                fclose(fp); \
        } \
} while (0)


#define LOG(fmt, args...) \
		do { \
				if (1) {\
					cprintf(fmt, ## args); \
					cprintf("\n"); \
				} \
		} while (0)


static int semId[MAX_SEM_LIST];

const char *applet_name;

MtcData mtcdata = {
    .boot.misc = 0,
};
MtcData *mtc = &mtcdata;

/**
 *  try to lock
 *
 *  @return none
 *
 */
void mtc_lock(SemList lockname)
{
    if (lockname<MAX_SEM_LIST) {
        down(semId[lockname]);
    }
    else {
        stdprintf("lockname(%d) is invalid\n",lockname);
    }
}

/**
 *  try to unlock
 *
 *  @return none
 *
 */
void mtc_unlock(SemList lockname)
{
    if (lockname<MAX_SEM_LIST) {
        up(semId[lockname]);
    }
    else {
        stdprintf("lockname(%d) is invalid\n",lockname);
    }
}

int mtc_get_semid(SemList lockname)
{
    if (lockname<MAX_SEM_LIST) {
        return semId[lockname];
    }
    else {
        stdprintf("lockname is invalid\n");
    }

    return UNKNOWN;
}

int read_pid_file(char *file)
{
    FILE *pidfile;
    char pidnumber[16];
    int pid = UNKNOWN;

    if ((pidfile = fopen(file, "r")) != NULL) {
        if (fscanf(pidfile, "%s", pidnumber) == 1) {
            if (isdigit(pidnumber[0])) {
                pid = my_atoi(pidnumber); 
            }
        }
        fclose(pidfile);
    }

    return pid;
}

static void mtc_save_pid(char *fpid)
{
    FILE *pidfile;

    mkdir("/var/run", 0755);
    if ((pidfile = fopen(fpid, "w")) != NULL) {
        fprintf(pidfile, "%d\n", getpid());
        (void) fclose(pidfile);
    }
}

static void mtc_shutdown()
{
    int pid = read_pid_file(MTC_PID);

    if (pid == UNKNOWN || pid != getpid()) {
        logger (LOG_INFO, "mtc pid is [%d], but pid is [%d]", getpid(), pid);
    }
    else {
        mtc_fsm_schedule(fsm_stop);
    }
}

static int mtc_get_sw_info(char *ver, char *build)
{
	FILE *fp;
	unsigned int v;
	unsigned int b;

	if((fp = fopen("/dev/mtdblock2", "rb")))
	{
		fseek(fp, 4, SEEK_SET);
		fread(&b, sizeof(unsigned int), 1, fp);
		fseek(fp, 24, SEEK_SET);
		fread(&v, sizeof(unsigned int), 1, fp);

		sprintf(ver, "%d", v);

		struct tm _tm;
		time_t *_t = (time_t *)&b;
		memcpy(&_tm, localtime(_t), sizeof(struct tm));
		strftime(build, 128, "%a %d %b %H:%M:%S %Y", &_tm);
	}
	fclose(fp);
	
	return 0;
}

/*
 * mtc_init
 * Only init cdb firstly
 */
static void mtc_init(void)
{
	char cpuclk[][4] = {"60","64","80","120","160","202","240","295","320","384","480","512","640","0"};
	char sysclk[][4] = {"60","64","80","120","150","160","0"};
	char hver[128] = {0};
	int hw_ver[4] = {0};
	int cpuclki, sysclki;
	char tmp[128] = {0};
	char vid[32] = {0};
	char pid[32] = {0};
	char sw_ver[128] = {0};
	char sw_build[128] = {0};

	mkdir("/tmp/run", 0755);
	exec_cmd("insmod /lib/modules/cdb.ko");
	cdb_load("/etc/config/cdb.define");
	cdb_load("/etc/config/rdb.define");
	if( is_cdb_empty() ) {
		cprintf("restore factory default...\n");
		cdb_load("/etc/config/default.cdb");
	} else {
		cdb_load(CDB_MTD);
	}
	
  boot_cdb_get("mac0", tmp);
  sprintf(mtc->boot.MAC0, "%s", tmp);

  boot_cdb_get("mac1", tmp);
  sprintf(mtc->boot.MAC1, "%s", tmp);

  boot_cdb_get("mac2", tmp);
  sprintf(mtc->boot.MAC2, "%s", tmp);

	boot_cdb_get("hver", hver);
	boot_cdb_get("id", tmp);
	strncpy(vid, tmp, 4);
	strncpy(pid, tmp+4, 4);

	sscanf(hver, "%3u.%3u.%3u.%3u", &hw_ver[3], &hw_ver[2], &hw_ver[1], &hw_ver[0]);
	cpuclki = (hw_ver[2]&0xF0)>>4;
	sysclki = (hw_ver[2]&0x0F);
  if (hw_ver[3] & 0x80) {
      mtc->boot.misc |= PROC_DBG;

	    mkdir("/tmp/log", 0755);
      exec_cmd("syslogd -S -s 500");
      exec_cmd("klogd -c 1");
      // default output to stdout, and report log in /var/log/messages
      setloglevel(LOG_DEBUG);
      openlogger(applet_name, 0);
  }

	mtc_get_sw_info(sw_ver, sw_build);

	cdb_set("$hw_ver", hver);
	cdb_set("$sw_ver", sw_ver);
	cdb_set("$sw_build", sw_build);
	cdb_set("$sw_vid", vid);
	cdb_set("$sw_pid", pid);	
	cdb_set("$chip_cpu", cpuclk[cpuclki]);
	cdb_set("$chip_sys", sysclk[sysclki]);

	cdb_merge();
	cdb_commit();
}

static void mtc_deinit(void)
{
    if (mtc->boot.misc & PROC_DBG) {
        exec_cmd("killall syslogd klogd");
        closelogger();
    }
}

static void mtc_start_boot(int sData, void *data)
{
    logger (LOG_INFO, "system boot");
	LOG("%s:%d *********** mtc_start_boot",__func__, __LINE__);
    mtc_proc_handle(ProcessBoot);
}

static void mtc_start_shutdown(int sData, void *data)
{
    logger (LOG_INFO, "system shutdown");
    mtc_proc_handle(ProcessShutdown);
}

int main(int argc, char **argv)
{
    int boot = 0;
    int i;

    if (argc > 1) {
        if (!strcmp(argv[1], "boot")) {
            boot = 1;
        }
        else if (!strcmp(argv[1], "shutdown")) {
            mtc_start_shutdown(0,0);
            return 0;
        }
    }
    if (fork() == 0) {
        //check PID
        if (read_pid_file(MTC_PID)!=UNKNOWN && read_pid_file(MTC_PID)!=getpid()) {
            stdprintf("MTC is already running?\n");
            return RET_ERR;
        }

        applet_name = strrchr(argv[0], '/');
        if (applet_name)
            applet_name++;
        else
            applet_name = argv[0];

        //write PID
        mtc_save_pid(MTC_PID);

        //create sem
        for (i=semWin; i<MAX_SEM_LIST; i++) {
            sem_create(ftok(MTC_PID, i), &semId[i]);
        }

        //setup signal
        signal(SIGTERM, mtc_shutdown);
        signal(SIGINT,	mtc_shutdown);
// atexit means that signal rises again when exit
//        atexit(mtc_shutdown);

        mtc_init();
        mtc_proc_init();

        mtcTimerInit();
        if (boot) {
            mtcTimerAdd(timerMtcStartBoot, mtc_start_boot, 0, NULL, 0);
        }
        mtc_inf();
        mtc_fsm();

        mtc_deinit();

        //remove sem
        int i;
        for (i=semWin; i<MAX_SEM_LIST; i++) {
            sem_delete(semId[i]);
        }

        //remove pid file
        remove(MTC_PID);
    }

    return 0;
}
