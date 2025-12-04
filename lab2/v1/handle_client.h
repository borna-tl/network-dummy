#ifndef HANDLE_CLIENT_H
#define HANDLE_CLIENT_H

#include <stddef.h>

int handle_client(int client_fd, const char *secret, size_t max_len);

#endif