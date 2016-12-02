/**
 * @file mtc_misc.c system help function
 *
 * @author  Frank Wang
 * @date    2010-10-06
 */

#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <fcntl.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <string.h>

#include "include/mtc.h"
#include "include/mtc_misc.h"

/**
 * @define LOG_IN_DEVICE
 * @brief config of log, default as disable
 */
#define LOG_IN_DEVICE   0

/**
 * @define BUFFER_SIZE
 * @brief size of buffer, default as 10240 bytes
 */
#define BUFFER_SIZE 10240

/**
 * @define DEBUGKILL
 * @brief config of debug, default as disable
 */
#define DEBUGKILL       0
/**
 * @define DEBUGSYSTEM
 * @brief config of debug, default as disable
 */
#define DEBUGSYSTEM     0

/**
 * @define sysprintf
 * @brief print message
 * @param fmt message format
 * @param args... args
 */
#define sysprintf(fmt, args...) do { \
            FILE *cfp = fopen(STDOUT, "a"); \
            if (cfp) { \
                fprintf(cfp, fmt, ## args); \
                fclose(cfp); \
            } \
        } while (0)

/**
 *  system command with parameters
 *
 *  @param *fmt interface name
 *  @param ...  args
 *
 *  @return number of command length
 *
 */
int __system(const char *file, const int line, const char *func, const char *fmt, ...)
{
    va_list args;
    int ret;
    char buf[MAX_COMMAND_LEN];

    va_start(args, fmt);
    vsprintf(buf, fmt,args);
    va_end(args);

#if LOG_IN_DEVICE
    mtcDebug("sys cmd:%s\n",buf);
#endif

#if DEBUGSYSTEM
    int pid = getpid();
    int ptid= my_gettid();
    sysprintf("\033[1m\033[%d;%dm[%-5d %-5d %-20s:%5d %16s()]:::\033[0m[S]%s\n",40, 30+(ptid%8)+1, pid, ptid, file,line,func,buf);
    ret = system(buf);
    sysprintf("\033[1m\033[%d;%dm[%-5d %-5d %-20s:%5d %16s()]:::\033[0m[E]%s\n",40, 30+(ptid%8)+1, pid, ptid, file,line,func,buf);
#else
    ret = system(buf);
#endif

    return ret;
}

/**
 *  kill command w/ debug message
 *
 *  @param pid
 *  @param sig
 *
 *  @return status code
 *
 */
int __kill(const char *file, const int line, const char *func, int pid, int sig)
{
    int ret;

    if ((pid == -1) || (pid ==0) || (pid ==1)) {
        stdprintf("kill %d\n", pid);
        ret = -1;
    }
    else {
#if DEBUGKILL
        unsigned int runpid = getpid();
        sysprintf("\033[1m\033[%d;%dm[%5d %-20s:%5d %16s()]:::\033[0m[S]kill %d %d\n",40, 30+(runpid%8)+1, runpid, file,line,func,pid,sig);
#endif
        ret = kill(pid,sig);
#if DEBUGKILL
        sysprintf("\033[1m\033[%d;%dm[%5d %-20s:%5d %16s()]:::\033[0m[E]kill %d %d[%d]\n",40, 30+(runpid%8)+1, runpid, file,line,func,pid,sig,ret);
#endif
    }

    return ret;
}

/**
 *  open a readable command pipe
 *
 *  @param *fmt interface name
 *  @param ...  args
 *
 *  @return File descriptor
 *
 */
FILE *_system_read(const char *fmt, ...)
{
    FILE *fp;

    va_list args;
    char buf[MAX_COMMAND_LEN];

    va_start(args, fmt);
    vsprintf(buf, fmt,args);
    va_end(args);

    //temp solution
    fflush(NULL);

    fp=popen(buf, "r");

#if LOG_IN_DEVICE
    mtcDebug("sys cmd:%s\n",buf);
    //stdprintf("sys cmd:%s\n",buf);
#endif

    if (fp!=NULL) {
        return fp;
    }

    return NULL;
}

/**
 *  close system command pipe
 *
 *  @param *fp  file pointer
 *
 *  @return none.
 *
 */
void _system_close(FILE *fp)
{
    pclose(fp);
}

/**
 *  print infomation message to stdout or console
 *
 *  @param *cfp fp
 *  @param *file file name
 *  @param *line line number
 *  @param *function function name
 *
 *  @return none.
 *
 */
#if DEBUGMSG
#include <time.h>

void __stdprintf_info__(FILE *cfp, const char *file, const int line, const char *function)
{
#if (DEBUGSLEEP || DEBUGWITHTIME)
    time_t now = time(NULL);
#endif
    fprintf(cfp,"["
#if (DEBUGSLEEP || DEBUGWITHTIME)
        "%4u "
#endif
        "%-20s:%5d %16s()]:: ",
#if (DEBUGSLEEP || DEBUGWITHTIME)
        (u32t )(now & 0x0fff), 
#endif
        file , line, function);
}
#endif

static int initialized;

/**
 *  get a random number
 *
 *  @return random number
 *
 */
unsigned int get_random()
{
    if (!initialized) {
        int fd;
        unsigned int seed;

        fd = open("/dev/urandom", 0);

        if (fd < 0 || read(fd, &seed, sizeof(seed)) < 0) {
            seed = (unsigned int)time(0);
        }

        if (fd >= 0) {
            close(fd);
        }

        srand(seed);
        initialized++;
    }
    return rand();
}

/**
 *  execute system command
 *  @param *fmt... format of command.
 *  @return the number of command characters.
 *
 */
int _execl(const char *fmt, ...)
{
    va_list args;
    int i;
    char buf[MAX_COMMAND_LEN];

    va_start(args, fmt);
    i = vsprintf(buf, fmt,args);
    va_end(args);

#if 1
    mtcDebug("sys cmd:%s\n",buf);
#endif

    if (vfork()==0) {
        execl("/bin/sh", "sh", "-c", buf, (char *)0);
        exit(0);
    }

    wait(NULL);

    return i;
}

/**
 *  file exist or not
 *  @param *file_name file name
 *  @return 1 for exist, 0 for no.
 *
 */
int _isFileExist(char *file_name)
{
    struct stat status;

    if ( stat(file_name, &status) < 0)
        return 0;

    return 1;
}

#include <sys/syscall.h>

pid_t my_gettid(void)
{
    return syscall(SYS_gettid);
}

int my_atoi(char *str)
{
    if (str == NULL) return 0;

    char *pEnd;

    return (int )strtol(str, &pEnd, 10);
}

/*
 *  translate string mac to value array
 *  @val(dst)
 *  @mac(src)
 *  @return
 */
int my_mac2val(u8 *val, s8 *mac)
{
    return sscanf(mac, "%hhx:%hhx:%hhx:%hhx:%hhx:%hhx", 
        &val[0], &val[1], &val[2], &val[3], &val[4], &val[5]);
}

/*
 *  translate value array to string mac
 *  @mac(dst)
 *  @val(src)
 *  @return
 */
int my_val2mac(s8 *mac, u8 *val)
{
    return sprintf(mac, "%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x", 
        val[0], val[1], val[2], val[3], val[4], val[5]);
}

