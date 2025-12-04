#ifndef ALARM_HANDLER_H
#define ALARM_HANDLER_H

#include <signal.h>

extern volatile sig_atomic_t alarm_sent_off;

void myalrmhandler(int sig);

#endif