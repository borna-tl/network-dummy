#define _POSIX_C_SOURCE 200809L
#include "sigint_handler.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>

volatile sig_atomic_t alarm_sent_off = 0;

void myalrmhandler(int sig) {
    printf("\nServer response timeout (3 seconds elapsed). No response received.\n");
    alarm_sent_off = 1;
}
