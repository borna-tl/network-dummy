#ifndef ALARM_HANDLER_H
#define ALARM_HANDLER_H

#include <signal.h>
#include <sys/time.h>

void alarm_handler(int sig);
void setup_alarm_handler(void);
void start_alarm_timer(int timeout_ms);
void stop_alarm_timer(void);

#endif
