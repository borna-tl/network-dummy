#ifndef SIGINT_HANDLER_H
#define SIGINT_HANDLER_H

#include <signal.h>

extern volatile sig_atomic_t should_exit;

void sigint_handler(int sig);

#endif