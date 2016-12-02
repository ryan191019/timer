/*
 * =====================================================================================
 *
 *       Filename:  mtc_client.h
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/15/2016 10:17:56 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */

#ifndef _MTC_CLIENT_H_
#define _MTC_CLIENT_H_
#include <sys/types.h>
#include "include/mtc_types.h"

typedef struct {
    int op;
    u8 args;
    u8 module;
    char *name;
}MtcCli;

#ifndef ARRAY_SIZE
#define ARRAY_SIZE(array) (sizeof(array) / sizeof(array[0]))
#endif

typedef int (*client_callback)(char *rbuf, int rlen);

int mtc_client_call(MtcPkt *packet, client_callback ccb);
int mtc_client_simply_call(int op_code, client_callback ccb);

#endif
