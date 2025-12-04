#define _POSIX_C_SOURCE 200809L
#include "install_sigint_handler.h"
#include "sigint_handler.h"
#include <signal.h>
#include <stdio.h>

int install_sigint_handler(void) 
{
    struct sigaction sa;
    sa.sa_handler = sigint_handler;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0; 
    return sigaction(SIGINT, &sa, NULL);
}