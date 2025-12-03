#define _POSIX_C_SOURCE 200809L
#include "process_ping_requests.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PACKET_SIZE 10 //6-byte secret key followed by 4 bytes that encode a 32-bit integer of type unsigned int. 

void process_ping_requests(int sockfd, const char *secret) {
    while (1) {
        char buffer[PACKET_SIZE];
        struct sockaddr_in6 client_addr;
        memset(&client_addr, 0, sizeof(client_addr));
        socklen_t client_len = sizeof(client_addr);


        ssize_t n = recvfrom(sockfd, buffer, PACKET_SIZE, 0, 
                            (struct sockaddr *)&client_addr, &client_len);
        if (n == -1) {
            perror("recvfrom");
            continue;
        }

        if (strncmp(buffer, secret, 6) != 0) {
            printf("Invalid secret");
            continue;
        }

        ssize_t sent = sendto(sockfd, buffer, PACKET_SIZE, 0,
                             (struct sockaddr *)&client_addr, client_len);
        if (sent == -1) {
            perror("sendto");
        }
    }
}