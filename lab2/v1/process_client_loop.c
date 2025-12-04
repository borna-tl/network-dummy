#define _POSIX_C_SOURCE 200809L
#include "process_client_loop.h"
#include "handle_client.h"
#include "sigint_handler.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/select.h>


void process_client_loop(int server_fd, const char *secret, size_t max_len) {
    fd_set master_set, working_set;
    int max_sd;

    FD_ZERO(&master_set);
    max_sd = server_fd;
    FD_SET(server_fd, &master_set);

    while (!should_exit) {
        working_set = master_set;
        //select between all fds
        int activity = select(max_sd + 1, &working_set, NULL, NULL, NULL);
        
        if (activity < 0) {
            if (errno == EINTR && should_exit) {
                break;
            }
            perror("select");
            break;
        }

        for (int i = 0; i <= max_sd; i++) {
            if (FD_ISSET(i, &working_set)) {
                if (i == server_fd) {
                    // new client connection
                    struct sockaddr_in client_address;
                    socklen_t address_len = sizeof(client_address);
                    int client_fd = accept(server_fd, (struct sockaddr *)&client_address, &address_len);
                    
                    if (client_fd == -1) {
                        if (errno == EINTR && should_exit) {
                            break;
                        }
                        perror("accept");
                        continue;
                    }

                    FD_SET(client_fd, &master_set);
                    if (client_fd > max_sd) {
                        max_sd = client_fd;
                    }

                    printf("New client connected: fd=%d\n", client_fd);
                } else {
                    // existing client sending data
                    int result = handle_client(i, secret, max_len);
                    if (result <= 0) {
                        close(i);
                        FD_CLR(i, &master_set);
                    }
                }
            }
        }

    }

    // cleanup: close all client connections
    for (int i = 0; i <= max_sd; i++) {
        if (FD_ISSET(i, &master_set) && i != server_fd) {
            close(i);
        }
    }
}