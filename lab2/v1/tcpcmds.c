#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "install_sigint_handler.h"
#include "setup_server.h"
#include "process_client_loop.h"
#include "sigint_handler.h"

#define MAXX 25 + 6 + 1

int main(int argc, char *argv[]) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <portnum> <secret>\n", argv[0]);
        exit(1);
    }

    int port = atoi(argv[1]);
    char secret[7];
    strncpy(secret, argv[2], 6);
    secret[6] = '\0';

    if (strlen(argv[2]) != 6) {
        fprintf(stderr, "Error: secret must be exactly 6 characters\n");
        exit(1);
    }

    if (install_sigint_handler() == -1) {
        perror("sigaction");
        exit(1);
    }

    int server_fd = setup_server(port);
    if (server_fd == -1) {
        fprintf(stderr, "Failed to setup server\n");
        exit(1);
    }

    printf("Server is running on port %d...\n", port);

    process_client_loop(server_fd, secret, MAXX);

    close(server_fd);
    printf("Server closed.\n");
    return 0;
}