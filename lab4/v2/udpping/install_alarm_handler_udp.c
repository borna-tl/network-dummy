#define _POSIX_C_SOURCE 200809L
#include "install_alarm_handler_udp.h"
#include "alarm_handler_udp.h"
#include <signal.h>
#include <stdio.h>

int install_alarm_handler(void) {
    struct sigaction sa;
    sa.sa_handler = alarm_handler_udp;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    return sigaction(SIGALRM, &sa, NULL);
}