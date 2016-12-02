
/*
 * @file mtc_ipc.h
 *
 * @Author  Frank Wang
 * @Date    2010-10-06
 */

#ifndef _MTC_IPC_H_
#define _MTC_IPC_H_

#include <sys/types.h>
#include <sys/ipc.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>

#include "mtc_types.h"
#include "mtc_misc.h"

/********************************************************************************************************/
/***********************************************SEM UTILITY**********************************************/
/********************************************************************************************************/

/**
 * @brief do "V" operation
 * @param semid semaphore id
 * @return RET_OK for success and RET_ERR for other else.
 */
Ret upup(IN int semid, char *c1, int d, const char *c2, unsigned char print);

/**
 * @define up
 * @brief sem up operation
 */
#define up(semid)           upup(semid, __FILE__ , __LINE__, __FUNCTION__, 0)
/**
 * @define up_debug
 * @brief sem up operation with debug
 */
#define up_debug(semid)     upup(semid, __FILE__ , __LINE__, __FUNCTION__, 1)

/**
 * @brief do "P" operation
 * @param semid semid semaphore id
 * @return RET_OK for success and RET_ERR for other else.
 */
Ret downdown(IN int semid, char *c1, int d, const char *c2, unsigned char print);
/**
 * @define down
 * @brief sem down operation
 */
#define down(semid)         downdown(semid, __FILE__ , __LINE__, __FUNCTION__, 0)
/**
 * @define down_debug
 * @brief sem down operation with debug
 */
#define down_debug(semid)   downdown(semid, __FILE__ , __LINE__, __FUNCTION__, 1)

/**
 * @brief create semaphore
 * @param *key semaphore key (IPC Key)
 * @param *semid when create success, you'll get a semaphore id in this field
 * @return RET_OK for success and RET_ERR for other else.
 */
Ret sem_create(IN int key, OUT int *semid);


/**
 * @brief delete semaphore by semaphore id
 * @param semid semaphore id
 * @return always Return RET_OK.
 */
Ret sem_delete(int semid);

/********************************************************************************************************/
/***********************************************SHM UTILITY**********************************************/
/********************************************************************************************************/

/**
 * @brief create share memory
 * @param key share memory key (IPC Key)
 * @param size share memory size that you want to create
 * @param *shmid when create success, you'll get a share memory id in this field
 * @return RET_OK for success and RET_ERR for other else.
 */
Ret shm_create(IN int key, IN unsigned int size, OUT int *shmid);


/**
 * @define sgm_attach
 * @brief attach share memory  
 * @param ptr   user point
 * @param type  point prototype
 * @param shmid share memory id
 */
#define shm_attach(ptr,type,shmid)  \
    ((int )(ptr = (type *)shmat(shmid, NULL, 0))!=-1) ? RET_OK : RET_ERR 

/**
 * @define shm_clrat
 * @brief clear share memory   
 * @param ptr   user point
 * @param size  point prototype 
 */
#define shm_clear(ptr,size) \
    memset(ptr,0,size)

/**
 * @define shm_deattach
 * @brief de-attach share memory. If you want to drop this share area, you can call shm_deattach(ptr) directly.
 * @param ptr   user point
 * @return RET_OK for success and RET_ERR for other else.
 */
#define shm_deattach(ptr)  \
    shmdt(ptr)==0 ? RET_OK : RET_ERR 

/** 
 * @define shm_delete
 * @brief de-attach share memory and free this area. 1.de-attach share memory 2.free this area
 * @param ptr   user point
 * @param shmid share memory id
 * @return RET_OK for success and RET_ERR for other else.
 */
#define shm_delete(ptr,shmid)  \
    shmdt(ptr)==0 ? ((shmctl(shmid,IPC_RMID,NULL) == 0) ? RET_OK : RET_ERR) : RET_ERR

#endif

