/*
 * =====================================================================================
 *
 *       Filename:  loop.c
 *
 *    Description:  
 *
 *        Version:  1.0
 *        Created:  11/21/2016 10:14:18 AM
 *       Revision:  none
 *       Compiler:  gcc
 *
 *         Author:  YOUR NAME (), 
 *   Organization:  
 *
 * =====================================================================================
 */
#include <stdio.h>
#include <unistd.h>
#include <signal.h>

static int run = 1;

static void uh_sigterm(int sig)
{
	run = 0;
}

int main(int argc, char *argv[])
{
	struct sigaction sa;
	sa.sa_handler = uh_sigterm;
	sigaction(SIGINT,  &sa, NULL);
	sigaction(SIGTERM, &sa, NULL);

    while (run) {
        sleep(1);
    }
    printf("loop break\n");
    return 0;
}


