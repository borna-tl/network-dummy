#ifndef ALARM_HANDLER_UDP_H
#define ALARM_HANDLER_UDP_H

#include <signal.h>

extern volatile sig_atomic_t alarm_sent_off;

void alarm_handler_udp(int sig);

#endif