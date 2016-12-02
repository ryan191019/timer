
/*
 * @file mtc_misc.h
 *
 * @Author  Frank Wang
 * @Date    2015-10-06
 */

#ifndef _MTC_MISC_H_
#define _MTC_MISC_H_

#define DEBUGMSG        0
#define DEBUGSLEEP      0
#define DEBUGWITHTIME   0

#define MAX_COMMAND_LEN        1024

/**
 * @define CONSOLE
 * @brief console path
 */
#define CONSOLE        "/dev/console"

/**
 * @define STDOUT
 * @brief stdout path
 */
#define STDOUT         CONSOLE

/**
 * @define stdprintf
 * @brief print message to stdout
 */
#if DEBUGMSG
void __stdprintf_info__(FILE *cfp, const char *file, const int line, const char *function);
#define stdprintf(fmt, args...) do { \
            FILE *cfp = fopen(STDOUT, "a"); \
            if (cfp) { \
                __stdprintf_info__(cfp,__FILE__ , __LINE__, __FUNCTION__); \
                fprintf(cfp, fmt, ## args); \
                fclose(cfp); \
            } \
        } while (0)
#else
#define stdprintf(fmt, args...) do { \
            FILE *cfp = fopen(STDOUT, "a"); \
            if (cfp) { \
                fprintf(cfp, fmt, ## args); \
                fclose(cfp); \
            } \
        } while (0)
#endif
/**
 * @define mtcDebug
 * @brief print debug message
 */
#define mtcDebug(fmt, args...) do { \
            FILE *cfp = fopen(CONSOLE, "a"); \
            if (cfp) { \
                fprintf(cfp, fmt, ## args); \
                fclose(cfp); \
            } \
        } while (0)

#define _system(fmt, args...)   __system(__FILE__ , __LINE__, __FUNCTION__, fmt, ## args)
#define mtc_kill(pid,sig)       __kill(__FILE__ , __LINE__, __FUNCTION__,pid,sig)

/**
 *  system command with parameters
 *
 *  @param *fmt interface name
 *  @param ...  args
 *
 *  @return number of command length
 *
 */
int __system(const char *file, const int line, const char *func, const char *fmt, ...);

/**
 *  kill command w/ debug message
 *
 *  @param pid
 *  @param sig
 *
 *  @return status code
 *
 */
int __kill(const char *file, const int line, const char *func, int pid, int sig);

/**
 *  open a readable command pipe
 *
 *  @param *fmt interface name
 *  @param ...  args
 *
 *  @return File descriptor
 *
 */
FILE *_system_read(const char *fmt, ...);

/**
 *  close system command pipe
 *
 *  @param *fmt interface name
 *  @param ...  args
 *
 *  @return none.
 *
 */
void _system_close(FILE *fp);

/**
 *  get a random number
 *
 *  @return random number
 *
 */
unsigned int get_random();

/**
 *  execute system command
 *  @param *fmt... format of command.
 *  @return the number of command characters.
 *
 */
int _execl(const char *fmt, ...);


/**
 *  file exist or not
 *  @param *file_name file name
 *  @return 1 for exist, 0 for no.
 *
 */
int _isFileExist(char *file_name);

#if DEBUGMSG
#define BUG_ON()    stdprintf("CHECK!!\n")
#define OOM()       stdprintf("OOM!!\n")
#define DBG()       stdprintf("DEBUG!!\n")
#define DBGM(msg)   stdprintf("%s\n",msg)
#else
#define BUG_ON()
#define OOM()
#define DBG()
#define DBGM(msg)
#endif

#define NI(str)     stdprintf("Need Implement:[%s]\n", str);

#if DEBUGSLEEP
#define my_sleep(sec)   stdprintf("sleep %d sec\n",sec);    sleep(sec)
#define my_usleep(usec) stdprintf("usleep %d usec\n",usec); usleep(usec)
#else
#define my_sleep(sec)   sleep(sec)
#define my_usleep(usec) usleep(usec)
#endif

pid_t my_gettid(void);

int my_atoi(char *str);
int my_mac2val(u8 *val, s8 *mac);
int my_val2mac(s8 *mac, u8 *val);

#endif

