#ifndef PROCESS_CLIENT_LOOP_H
#define PROCESS_CLIENT_LOOP_H

#include <stddef.h>

void process_client_loop(int server_fd, const char *secret, size_t max_len);

#endif