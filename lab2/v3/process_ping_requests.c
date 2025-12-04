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
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        ssize_t n = recvfrom(sockfd, buffer, PACKET_SIZE, 0, 
                            (struct sockaddr *)&client_addr, &client_len);
        printf("Received %zd bytes from %s:%d\n", n,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        if (n == -1) {
            perror("recvfrom");
            continue;
        }

        if (strncmp(buffer, secret, 6) != 0) {
            printf("Invalid secret from client %s:%d\n",
                   inet_ntoa(client_addr.sin_addr),
                   ntohs(client_addr.sin_port));
            continue;
        }

        ssize_t sent = sendto(sockfd, buffer, PACKET_SIZE, 0,
                             (struct sockaddr *)&client_addr, client_len);
        printf("Sent %zd bytes back to %s:%d\n", sent,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));
        if (sent == -1) {
            perror("sendto");
        }
    }
}