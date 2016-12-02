#include "include/mtc_client.h"
#include "wdk.h"

int wdk_commit(int argc, char **argv)
{
    return mtc_client_simply_call(OPC_CMD_COMMIT, NULL);
}

