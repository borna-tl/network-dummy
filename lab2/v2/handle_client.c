#define _POSIX_C_SOURCE 200809L
#include "handle_client.h"
#include "execute_command.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
static size_t buffer_pos = 0;

int handle_client(int client_fd, const char *secret, size_t max_len) {
    char client_buffer[max_len];
    char c;
    ssize_t n = read(client_fd, &c, 1);
    
    if (n == -1) {
        perror("read from client");
        return -1;
    } else if (n == 0) {
        printf("Client fd=%d disconnected\n", client_fd);
        return 0;
    }

    if (c == '\n') {
        client_buffer[buffer_pos] = '\0';
        
        // check minimum length (6 for secret + 1 for space + at least 1 for command)
        if (buffer_pos < 8) {
            fprintf(stderr, "Request too short from fd=%d\n", client_fd);
            buffer_pos = 0;
            return 1;
        }

        // verify secret (first 6 characters)
        if (strncmp(client_buffer, secret, 6) != 0 || client_buffer[6] != ' ') {
            struct sockaddr_in addr;
            socklen_t addr_len = sizeof(addr);
            getpeername(client_fd, (struct sockaddr *)&addr, &addr_len);
            
            printf("Invalid secret from client %s:%d\n",
                   inet_ntoa(addr.sin_addr),
                   ntohs(addr.sin_port));
            
            buffer_pos = 0;
            return 1;
        }

        // extract command (everything after secret and space)
        size_t cmd_len = buffer_pos - 7;  // 6 for secret + 1 for space
        if (cmd_len == 0 || cmd_len > max_len) {
            fprintf(stderr, "Command length invalid from fd=%d (len=%zu)\n", client_fd, cmd_len);
            buffer_pos = 0;
            return 1;
        }

        char cmd[max_len];
        strncpy(cmd, client_buffer + 7, cmd_len);
        cmd[cmd_len] = '\0';
        execute_command(cmd, client_fd);
        buffer_pos = 0;
        return 1;
    } else {
        if (buffer_pos < sizeof(client_buffer) - 1) {
            client_buffer[buffer_pos++] = c;
        } else {
            perror("Buffer overflow");
            return -1;
        }
        return 1;
    }
}