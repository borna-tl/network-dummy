#define _POSIX_C_SOURCE 200809L
#include "setup_udp_client.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>

int setup_udp_client(const char *server_ip, int client_port) {
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin_family = AF_INET;
    client_addr.sin_addr.s_addr = INADDR_ANY;
    client_addr.sin_port = htons(client_port);

    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) {
        fprintf(stderr, "Error: Could not bind to %d\n", client_port);        
        close(sockfd);
        exit(1);
    }

    return sockfd;
}