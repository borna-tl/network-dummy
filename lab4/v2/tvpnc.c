#define _POSIX_C_SOURCE 200809L
#include "tvpnc.h"

int main(int argc, char *argv[]) {
    if (argc != 7) {
        fprintf(stderr, "Usage: %s <server_ip> <server_port> <secret> <client_ip> <dest_ip> <dest_port>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    char *secret = argv[3];
    char *client_ip = argv[4];
    char *dest_ip = argv[5];
    int dest_port = atoi(argv[6]);

    if (strlen(secret) != 6) {
        fprintf(stderr, "Error: secret must be exactly 6 characters\n");
        exit(EXIT_FAILURE);
    }

    // Create TCP socket
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Connect to server
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(server_port);
    if (inet_pton(AF_INET, server_ip, &server_addr.sin_addr) <= 0) {
        fprintf(stderr, "Invalid server IP address\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    if (connect(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("connect");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Connected to tvpns at %s:%d\n", server_ip, server_port);

    // Send connection request (2 bytes - value 315 in network byte order)
    uint16_t conn_req = htons(315);
    if (write(sockfd, &conn_req, 2) != 2) {
        perror("write");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Send secret key (6 bytes)
    if (write(sockfd, secret, 6) != 6) {
        perror("write");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Send destination IPv4 address (4 bytes)
    struct in_addr dest_addr;
    if (inet_pton(AF_INET, dest_ip, &dest_addr) <= 0) {
        fprintf(stderr, "Invalid destination IP address\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    if (write(sockfd, &dest_addr.s_addr, 4) != 4) {
        perror("write");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Send destination port (2 bytes in network byte order)
    uint16_t dest_port_net = htons(dest_port);
    if (write(sockfd, &dest_port_net, 2) != 2) {
        perror("write");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    
    // Send source IPv4 address (4 bytes)
    struct in_addr source_addr;
    if (inet_pton(AF_INET, client_ip, &source_addr) <= 0) {
        fprintf(stderr, "Invalid client IP address\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }
    if (write(sockfd, &source_addr.s_addr, 4) != 4) {
        perror("write");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Receive secret + UDP port from server (8 bytes total)
    char response[8];
    ssize_t n = read(sockfd, response, 8);
    if (n != 8) {
        fprintf(stderr, "Failed to receive response from server\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Verify secret
    if (strncmp(response, secret, 6) != 0) {
        fprintf(stderr, "Invalid secret in response\n");
        close(sockfd);
        exit(EXIT_FAILURE);
    }

    // Extract port number
    uint16_t udp_port;
    memcpy(&udp_port, response + 6, 2);
    udp_port = ntohs(udp_port);

    printf("Tunnel established. UDP port: %d\n", udp_port);
    printf("Use this command for udppingc:\n");
    printf("udppingc %s %d <secret> <portnum2> <pcount>\n", server_ip, udp_port);

    // Keep connection open
    printf("Press Enter to terminate tunnel...\n");
    getchar();

    // Send termination request (6-byte secret)
    if (write(sockfd, secret, 6) == 6) {
        printf("Termination request sent\n");
    }

    close(sockfd);
    return 0;
}