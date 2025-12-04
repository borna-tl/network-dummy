#define _POSIX_C_SOURCE 200809L
#include "install_alarm_handler.h"
#include "alarm_handler.h"
#include <signal.h>
#include <stdio.h>

int install_alarm_handler(void) 
{
    // Register SIGALRM handler
    struct sigaction sa;
    sa.sa_handler = myalrmhandler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    return sigaction(SIGALRM, &sa, NULL);
}