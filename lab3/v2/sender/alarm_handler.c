#define _POSIX_C_SOURCE 200809L
#include "alarm_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <signal.h>

static struct itimerval timer;

void alarm_handler(int sig) {
}

void setup_alarm_handler(void) {
    struct sigaction sa_alarm;
    memset(&sa_alarm, 0, sizeof(sa_alarm));
    sa_alarm.sa_handler = alarm_handler;
    sigemptyset(&sa_alarm.sa_mask);
    sa_alarm.sa_flags = 0;

    if (sigaction(SIGALRM, &sa_alarm, NULL) == -1) {
        perror("sigaction (SIGALRM)");
        exit(EXIT_FAILURE);
    }
}


void start_alarm_timer(int timeout_ms) {
    timer.it_value.tv_sec = timeout_ms / 1000;
    timer.it_value.tv_usec = (timeout_ms % 1000) * 1000;
    timer.it_interval.tv_sec = 0;
    timer.it_interval.tv_usec = 0;

    if (setitimer(ITIMER_REAL, &timer, NULL) == -1) {
        perror("setitimer");
        exit(EXIT_FAILURE);
    }
}


void stop_alarm_timer(void) {
    memset(&timer, 0, sizeof(timer));
    setitimer(ITIMER_REAL, &timer, NULL);
}
