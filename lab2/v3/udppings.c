#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include "setup_udp_server.h"
#include "process_ping_requests.h"

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

    int sockfd = setup_udp_server(port);
    if (sockfd == -1) {
        fprintf(stderr, "Failed to setup UDP server\n");
        exit(1);
    }

    printf("Server is running on port %d...\n", port);

    process_ping_requests(sockfd, secret);

    close(sockfd);
    return 0;
}