#define _POSIX_C_SOURCE 200809L
#include "sigint_handler.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>

volatile sig_atomic_t should_exit = 0;

void sigint_handler(int sig) {
    printf("\nServer received SIGINT, shutting down...\n");
    fflush(stdout);
    should_exit = 1;
}