#define _POSIX_C_SOURCE 200809L
#include "alarm_handler_udp.h"
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>

volatile sig_atomic_t alarm_sent_off = 0;

void alarm_handler_udp(int sig) {
    // printf("\nServer response timeout (3 seconds elapsed). No response received.\n"); //removed to get accurate ping.
    alarm_sent_off = 1;
}