#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define MAXX 25 + 6 + 1  // 6 for secret, 1 for space, 25 for command, 1 for newline

int connect_server(char *server_ip, int port) {
    int fd;
    struct sockaddr_in server_address;
    
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {        
        perror("socket");
        return -1;
    }
    
    memset(&server_address, 0, sizeof(server_address));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(port);
    server_address.sin_addr.s_addr = inet_addr(server_ip);

    if (connect(fd, (struct sockaddr *)&server_address, sizeof(server_address)) < 0) {
        perror("connect");
        close(fd);
        return -1;
    }

    return fd;
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <servip> <portnum> <secret>\n", argv[0]);
        exit(1);
    }

    char *server_ip = argv[1];
    int port = atoi(argv[2]);
    char secret[7];
    strncpy(secret, argv[3], 6);
    secret[6] = '\0';

    if (strlen(argv[3]) != 6) {
        fprintf(stderr, "Error: secret must be exactly 6 characters\n");
        exit(1);
    }

    int sock_fd = connect_server(server_ip, port);
    if (sock_fd == -1) {
        fprintf(stderr, "Failed to connect to server\n");
        exit(1);
    }

    printf("Connected to server at %s:%d\n", server_ip, port);

    char line[128];
    while (1) {
        printf("enter command: ");
        fflush(stdout);

        if (fgets(line, sizeof(line), stdin) == NULL) {
            if (errno == EINTR) {
                clearerr(stdin);
                continue;
            }
            break;
        }

        size_t len = strlen(line);

        if (len == 0 || len == 1) {
            continue;
        }
        if (line[len-1] == '\n') {
            line[len-1] = '\0';
            len--;
        }
        // Total message length check
        if (len > MAXX) {
            printf("Error: Total length of the input string may not exceed %d characters. Please try again.\n", MAXX);
            continue;
        }

        // Parse input: should be exactly 2 tokens (secret and command)
        char input_copy[128];
        strncpy(input_copy, line, sizeof(input_copy));
        
        char *token_1 = strtok(input_copy, " \t");
        char *token_2 = strtok(NULL, " \t");
        char *token_3 = strtok(NULL, " \t");
        
        if (token_1 == NULL || token_2 == NULL) {
            printf("Error: Please enter both secret and command (e.g., '123456 ls').\n");
            continue;
        }
        
        if (token_3 != NULL) {
            printf("Error: Command cannot have arguments. Enter only secret and command.\n");
            continue;
        }

        // prepare message: input as-is + newline
        char msg[MAXX + 2];
        memcpy(msg, line, len);
        msg[len] = '\n';
        ssize_t w = write(sock_fd, msg, len + 1);
        if (w == -1) {
            if (errno == EPIPE) {
                fprintf(stderr, "Server closed the connection. Exiting.\n");
            } else {
                perror("write");
            }
            close(sock_fd);
            exit(1);
        }
        else if (w != len + 1) {
            fprintf(stderr, "Write not complete. Exiting.\n");
            close(sock_fd);
            exit(1);
        }

        printf("Request sent.\n");

        // Try to read from server to detect if connection is closed
        // This is a non-blocking check to see if server has closed the socket
        char ack[1];
        ssize_t r = recv(sock_fd, ack, sizeof(ack), MSG_DONTWAIT | MSG_PEEK);
        if (r == 0) {
            // Server closed the connection
            fprintf(stderr, "Server closed the connection. Exiting.\n");
            close(sock_fd);
            exit(1);
        }
    }
    
    close(sock_fd);
    return 0;
}