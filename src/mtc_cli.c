
/**
 * @file mtc_cli.c
 *
 * @author 
 * @date 
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>

#include <shutils.h>
#include "include/mtc_client.h"

extern const MtcCli mtcCli[];
extern const int mtcCliCount;

void usage()
{
    int i, j;

    printf("\n");
    printf("Usage:\n");
    printf("    mtc_cli KEY_EVENT [COUNT]\n");
    printf("    mtc_cli CMD_EVENT \n");
    printf("    mtc_cli MOD_EVENT [start|stop|restart|dbg|undbg]\n");
    printf("\n");
    printf("KEY_EVENT:\n");
    printf("    as|bs|cs|ds - short A|B|C|D key\n");
    printf("    hl|hr COUNT - hwheel left|right count\n");
    printf("CMD_EVENT:\n");
    printf("    commit      - cdb commit\n");
    printf("    save        - cdb save\n");
    printf("    shutdown    - shutdown without reboot\n");
    printf("    wanipup     - wan ip up\n");
    printf("    wanipdown   - wan ip down\n");
    printf("    ocfgarg     - ocfg with argument 1\n");
    printf("    info        - get mtc information\n");
    printf("MOD_EVENT:\n");
    printf("    wan restart - wan service restart\n");
    printf("    wan dbg     - wan enable dbg\n");
    printf("    wan undbg   - wan disable dbg\n");
    printf("MODULE:\n");
    printf("    ");
    for (i=0, j=0; i<mtcCliCount; i++) {
        if (mtcCli[i].module) {
            if (j > 6) {
                printf("\n    ");
                j = 0;
            }
            printf("%s ", mtcCli[i].name);
            j++;
        }
    }
    printf("\n");
}

int parse_args(int argc, char **argv, MtcPkt *pkt)
{
    int i;

    for (i=0; i<mtcCliCount; i++) {
        if (mtcCli[i].args < 2) {
            printf("[%s] args must be >= 2\n", mtcCli[i].name);
            continue;
        }
        if ((argc >= mtcCli[i].args) && 
            (strcmp(argv[1], mtcCli[i].name) == RET_OK)) {
            pkt->head.opc = mtcCli[i].op;
            if (mtcCli[i].args > 2) {
                pkt->head.arg = atoi(argv[mtcCli[i].args - 1]);
                pkt->head.len = strlen(argv[mtcCli[i].args - 1]) + 1;
                strcpy((char *)&pkt->data, argv[mtcCli[i].args - 1]);
            }
            
            if (pkt->head.opc < OPC_KEY_MIN)
                return RET_ERR;
            else if (pkt->head.opc <= OPC_KEY_MAX)
                pkt->head.ifc = INFC_KEY;
            else if (pkt->head.opc <= OPC_CMD_MAX)
                pkt->head.ifc = INFC_CMD;
            else if (pkt->head.opc <= OPC_DAT_MAX)
                pkt->head.ifc = INFC_DAT;

            return RET_OK;
        }
    }

    return RET_ERR;
}

int cli_callback(char *rbuf, int rlen)
{
    printf("%s\n", rbuf);
    return 0;
}

int main(int argc, char *argv[])
{
    MtcPkt *packet;
    struct timeval tm_now;
    struct tm *tm_info;
    char *log, *logh;
    int len, i;
    int ret = RET_ERR;

    setloglevel(LOG_DEBUG);
    openlogger(argv[0], 0);
    gettimeofday(&tm_now, NULL);
    tm_info = localtime(&tm_now.tv_sec);

    for (i=0,len=0;i<argc;i++) {
        len += strlen(argv[i]);
    }

    if((logh = log = calloc(1, len + argc + 50))) {
        log += strftime(log, 50, "%Y-%m-%d %H:%M:%S", tm_info);
        log += sprintf(log, ".%6.6d [", (int)tm_now.tv_usec);
        for (i=0;i<argc;i++) {
            log += sprintf(log, "%s ", argv[i]);
        }
        *(log - 1) = 0;
        logger (LOG_INFO, "%s]", logh);
        free(logh);
    }

    if(!(packet = calloc(1, sizeof(MtcPkt) + len))) {
        goto err;
    }

    if(parse_args(argc, argv, packet) != RET_OK) {
        usage();
        goto err;
    }

    ret = mtc_client_call(packet, cli_callback);

err:
    if (packet) {
        free(packet);
    }

    closelogger();

    return ret;
}

