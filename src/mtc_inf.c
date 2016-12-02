/**
 * @file mtc_inf.c
 *
 * @author  Frank Wang
 * @date    2015-10-06
 */

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stddef.h>
#include <errno.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include <time.h>
#include <fcntl.h>
#include <signal.h>

#include "include/mtc_types.h"
#include "include/mtc_linklist.h"
#include "include/mtc_misc.h"
#include "include/mtc_event.h"
#include "include/mtc_proc.h"
#include "include/mtc.h"
#include "include/mtc_threadpool.h"

#define MAX_CONNECTION_SUPPORT    32

extern MtcData *mtc;

struct InfJob;
typedef unsigned char (*job_write_callback)(void *priv, char *buf, int size);
typedef int (*job_close_callback)(void *priv, int dofree);

typedef struct
{
    struct list_head list;
    MtcPkt packet;
    int sockfd;
    unsigned char thread:1;
    unsigned char reservd:7;
    job_write_callback jobwrite;
    job_close_callback jobclose;
}InfJob;

threadpool_t *pool = NULL;

int inf_sock = UNKNOWN;

static const char *opc_data_name[] = {
    [OPC_DAT_IDX(OPC_DAT_INFO)]       = "info",
    [OPC_DAT_IDX(OPC_DAT_MAX)]        = NULL
};

void sendData(OpCodeData opc, InfJob *job)
{
    if (opc > OPC_DAT_MAX || opc < OPC_DAT_MIN) {
        logger (LOG_ERR, "Execute Data(0x%x) out of range", opc);
    }
    else {
        logger (LOG_INFO, "Execute Data:[%s](0x%x)", opc_data_name[OPC_DAT_IDX(opc)], opc);
        switch(opc) {
            case OPC_DAT_INFO:
            {
                char info[MSBUF_LEN];

                snprintf(info, sizeof(info), "MAC0=%s\nMAC1=%s\nMAC2=%s\n", 
                    mtc->boot.MAC0, mtc->boot.MAC1, mtc->boot.MAC2);
                job->jobwrite(job, info, strlen(info));
                snprintf(info, sizeof(info), "LAN=%s\nWAN=%s\nWANB=%s\n", 
                    mtc->rule.LAN, mtc->rule.WAN, mtc->rule.WANB);
                job->jobwrite(job, info, strlen(info));
                snprintf(info, sizeof(info), "STA=%s\nAP=%s\nBRSTA=%s\nBRAP=%s\nIFPLUGD=%s\n", 
                    mtc->rule.STA, mtc->rule.AP, mtc->rule.BRSTA, mtc->rule.BRAP, mtc->rule.IFPLUGD);
                job->jobwrite(job, info, strlen(info));
                snprintf(info, sizeof(info), "LMAC=%s\nWMAC=%s\nSTAMAC=%s\n", 
                    mtc->rule.LMAC, mtc->rule.WMAC, mtc->rule.STAMAC);
                job->jobwrite(job, info, strlen(info));
                snprintf(info, sizeof(info), "HOSTAPD=%d\nWPASUP=%d\n", 
                    mtc->rule.HOSTAPD, mtc->rule.WPASUP);
                job->jobwrite(job, info, strlen(info));
                snprintf(info, sizeof(info), "OMODE=%d\nWMODE=%d\n", 
                    mtc->rule.OMODE, mtc->rule.WMODE);
                job->jobwrite(job, info, strlen(info));
            }
            break;

            default:
            {
                logger (LOG_ERR, "Execute Data(0x%x) unknown", opc);
            }
            break;

        }
    }
}

int socket_close(int sock)
{
    close(sock);
    return 0;
}

static unsigned char setup_inf_sock()
{
    if (inf_sock!=UNKNOWN) {
        socket_close(inf_sock);
    }

    //create socket
    if ((inf_sock=socket(AF_UNIX,SOCK_STREAM,0))<0) {
        return FALSE;
    }

    unlink(MTCSOCK);

    struct sockaddr_un addr;
    bzero(&addr,sizeof(addr));
    addr.sun_family = AF_UNIX;
    strcpy(addr.sun_path, MTCSOCK);
    int len = offsetof(struct sockaddr_un, sun_path) + strlen(MTCSOCK);

bind_again:
    if (bind(inf_sock, (struct sockaddr *)&addr, len) < 0) {
        //perror("INF@bind:");
        my_sleep(1);
        goto bind_again;
    }

listen_again:
    if (listen(inf_sock, MAX_CONNECTION_SUPPORT) < 0) {
        //perror("INF@listen:");
        my_sleep(1);
        goto listen_again;
    }

    logger (LOG_INFO, "Setup inf socket success");

    return TRUE;
}


static unsigned char jobRead(int sock, char *buf, int size)
{
    int ret;

    ret = read(sock, buf, size);

    if(ret == -1)
        return FALSE;

    return TRUE;
}

static unsigned char jobWrite(void *priv, char *buf, int size)
{
    InfJob *job = (InfJob *)priv;
    int cfd = job->sockfd;
    return write(cfd, buf, size);
}

static int jobClose(void *priv, int dofree)
{
    InfJob *job = (InfJob *)priv;
    int cfd = job->sockfd;

    if (cfd) {
        socket_close(cfd);
        job->sockfd = 0;
    }
    if (dofree) {
        if (job->packet.data != NULL)
            free(job->packet.data);
        free(job);
    }

    return 0;
}

static void exec_job(void *arg)
{
    InfJob *job = arg;
    PHdr *head = &job->packet.head;

    switch(job->packet.head.ifc) {
        case INFC_KEY:    // interface for key
        {
            //stdprintf("Process an INF_KEY interface!!");
            //handle_inf_key(job->sockfd, &job->packet.head, job->packet.data);

            KeyCB key_cb;

            memset(&key_cb, 0, sizeof(KeyCB));
            key_cb.key = head->opc;
            key_cb.data = head->arg;
            job->jobclose(job, 0);
            sendEvent(EventKey, 0, &key_cb, sizeof(KeyCB), TRUE);
        }
        break;

        case INFC_CMD:    // interface for cmd
        {
            job->jobclose(job, 0);
            sendCmd(head->opc, job->packet.data);
        }
        break;

        case INFC_DAT:    // interface for data
        {
            sendData(head->opc, job);
        }
        break;

        default:
        {
            logger (LOG_ERR, "Process an unknown interface!!");
        }
        break;
    }

    job->jobclose(job, 1);
}


static void infHandler(void *arg)
{
    logger (LOG_INFO, "Thread Create[infHandler]");

    if (!setup_inf_sock()) {
        BUG_ON();
        return;
    }

    for (;;) {
        int cfd,sock_len;
        struct sockaddr_un sock_addr;

        memset((char *)&sock_addr, 0, sizeof(sock_addr));
        sock_len = sizeof(sock_addr);
        cfd = accept(inf_sock, (struct sockaddr *)&sock_addr,(socklen_t *)&sock_len);

        InfJob *job = calloc(1,sizeof(InfJob));
        if (job) {
            int size = sizeof(PHdr);
            list_init(&job->list);
            if (jobRead(cfd,(char *)&job->packet,size)) {
                if (job->packet.head.len) {   //recv data
                    job->packet.data = calloc(1,job->packet.head.len+1);
                    size = job->packet.head.len;
    
                    if (!jobRead(cfd,job->packet.data,size)) {
                        free(job->packet.data);
                        free(job);
                        socket_close(cfd);
                        logger (LOG_ERR, "Recv Fail, socket %d close", cfd);
                        continue;
                    }
                }
                //process job, 
                job->sockfd = cfd;
                job->jobwrite = jobWrite;
                job->jobclose = jobClose;
                fcntl(cfd, F_SETFD, FD_CLOEXEC);

                if (task_enqueue(pool, exec_job, job, 0)!=threadpool_success) {
                    job->jobclose(job, 1);
                    logger (LOG_ERR, "Task Enqueue Fail, socket %d close", cfd);
                }
            }
            else {
                socket_close(cfd);
                logger (LOG_ERR, "RECV, socket %d close", cfd);
            }
        }
        else {    //no memory
            socket_close(cfd);
            logger (LOG_ERR, "OOM, socket %d close", cfd);
        }
    }

    logger (LOG_INFO, "end of infHandler");

    return;
}

unsigned char mtc_inf()
{
    pool = threadpool_init(3, 128, 0);

    if ((pool == NULL)) {
        BUG_ON();
        return FALSE;
    }

    srand((unsigned)(time(0))); 

    if (task_enqueue(pool, infHandler, NULL, 0)!=threadpool_success) {
        BUG_ON();
        return FALSE;
    }

    return TRUE;
}


