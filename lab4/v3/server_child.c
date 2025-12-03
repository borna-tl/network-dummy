#include "tvpns.h"

void run_child_session(int session_idx, int client_sockfd, char *secret) {
    
    // Create first UDP socket (for client communication)
    int udp_client_sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (udp_client_sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // Bind to port starting at 55500
    struct sockaddr_in6 udp_client_addr;
    memset(&udp_client_addr, 0, sizeof(udp_client_addr));
    udp_client_addr.sin6_family = AF_INET6;
    udp_client_addr.sin6_addr = in6addr_any;

    int client_port = BASE_UDP_PORT;
    int bind_success = 0;
    for (int i = 0; i < MAX_BIND_ATTEMPTS; i++) { // you can try up to 100 ports
        udp_client_addr.sin6_port = htons(client_port);
        if (bind(udp_client_sockfd, (struct sockaddr *)&udp_client_addr, sizeof(udp_client_addr)) == 0) {
            bind_success = 1;
            break;
        }
        client_port++;
    }

    if (!bind_success) {
        fprintf(stderr, "Failed to bind UDP client socket\n");
        close(udp_client_sockfd);
        exit(EXIT_FAILURE);
    }

    // Send secret + port to client via TCP
    char response[8];
    memcpy(response, secret, 6);
    uint16_t port_net = htons(client_port);
    memcpy(response + 6, &port_net, 2);
    if (write(client_sockfd, response, 8) != 8) {
        perror("write");
        close(udp_client_sockfd);
        exit(EXIT_FAILURE);
    }

    printf("Child %d: UDP client socket bound to port %d\n", session_idx, client_port);

    // Create second UDP socket (for destination communication)
    int udp_dest_sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (udp_dest_sockfd == -1) {
        perror("socket");
        close(udp_client_sockfd);
        exit(EXIT_FAILURE);
    }

    // Bind to port starting at 57500
    struct sockaddr_in6 udp_tunnel_addr;
    memset(&udp_tunnel_addr, 0, sizeof(udp_tunnel_addr));
    udp_tunnel_addr.sin6_family = AF_INET6;
    udp_tunnel_addr.sin6_addr = in6addr_any;

    int tunnel_port = BASE_TUNNEL_PORT;
    bind_success = 0;
    for (int i = 0; i < MAX_BIND_ATTEMPTS; i++) {
        udp_tunnel_addr.sin6_port = htons(tunnel_port);
        if (bind(udp_dest_sockfd, (struct sockaddr *)&udp_tunnel_addr, 
                sizeof(udp_tunnel_addr)) == 0) {
            bind_success = 1;
            break;
        }
        tunnel_port++;
    }

    if (!bind_success) {
        fprintf(stderr, "Failed to bind UDP destination socket\n");
        close(udp_client_sockfd);
        close(udp_dest_sockfd);
        exit(EXIT_FAILURE);
    }

    forwardtab[session_idx].tunnelpt = tunnel_port;

    printf("Child %d: UDP tunnel socket bound to port %d\n", session_idx, tunnel_port);

    // Prepare destination address
    struct sockaddr_in6 dest_sockaddr;
    memset(&dest_sockaddr, 0, sizeof(dest_sockaddr));
    dest_sockaddr.sin6_family = AF_INET6;
    memcpy(&dest_sockaddr.sin6_addr, &forwardtab[session_idx].destaddr, sizeof(struct in6_addr));
    dest_sockaddr.sin6_port = htons(forwardtab[session_idx].destpt);

    char dest_str[INET6_ADDRSTRLEN];
    inet_ntop(AF_INET6, &forwardtab[session_idx].destaddr, dest_str, INET6_ADDRSTRLEN);

    // Use select to monitor all three sockets
    fd_set readfds;
    int max_fd = (client_sockfd > udp_client_sockfd) ? client_sockfd : udp_client_sockfd;
    max_fd = (udp_dest_sockfd > max_fd) ? udp_dest_sockfd : max_fd;

    struct sockaddr_in6 from_addr;
    socklen_t from_len;
    char buffer[BUFFER_SIZE];

    while (1) {
        FD_ZERO(&readfds);
        FD_SET(client_sockfd, &readfds);
        FD_SET(udp_client_sockfd, &readfds);
        FD_SET(udp_dest_sockfd, &readfds);

        int activity = select(max_fd + 1, &readfds, NULL, NULL, NULL);
        if (activity < 0 && errno != EINTR) {
            perror("select");
            break;
        }

        if (activity == 0) {
            continue;
        }
        ssize_t n = 0;
        // Check TCP socket for termination
        if (FD_ISSET(client_sockfd, &readfds)) {
            char term_secret[6];
            n = read(client_sockfd, term_secret, 6);
            if (n == 6 && strncmp(term_secret, secret, 6) == 0) {
                printf("Child %d: Termination request received. Exiting now...\n", session_idx);
                close(client_sockfd);
                close(udp_client_sockfd);
                close(udp_dest_sockfd);
                memset(&forwardtab[session_idx].sourceaddr, 0, sizeof(struct in6_addr));
                exit(EXIT_SUCCESS);
            } else if (n == 0) {
                // TCP connection closed
                printf("Child %d: TCP connection closed\n", session_idx);
                close(client_sockfd);
                close(udp_client_sockfd);
                close(udp_dest_sockfd);
                memset(&forwardtab[session_idx].sourceaddr, 0, sizeof(struct in6_addr));
                exit(EXIT_SUCCESS);
            }
        }

        // Check UDP client socket (data from client)
        if (FD_ISSET(udp_client_sockfd, &readfds)) {
            from_len = sizeof(from_addr);
            n = recvfrom(udp_client_sockfd, buffer, BUFFER_SIZE, 0,
                        (struct sockaddr *)&from_addr, &from_len);
            if (n > 0) {
                // Store the client's source port from the first packet
                if (forwardtab[session_idx].sourcept == 0) {
                    forwardtab[session_idx].sourcept = ntohs(from_addr.sin6_port);
                    char client_str[INET6_ADDRSTRLEN];
                    inet_ntop(AF_INET6, &from_addr.sin6_addr, client_str, INET6_ADDRSTRLEN);
                            printf("Child %d: Client source port set to %d from [%s]\n", 
                                   session_idx, forwardtab[session_idx].sourcept, client_str);
                }

                char from_str[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &from_addr.sin6_addr, from_str, INET6_ADDRSTRLEN);
                // printf("Child %d: Received %zd bytes from client [%s]:%d, forwarding to destination\n", session_idx, n, from_str, ntohs(from_addr.sin6_port));
                        
                        
                // Forward to destination
                ssize_t sent = sendto(udp_dest_sockfd, buffer, n, 0,
                        (struct sockaddr *)&dest_sockaddr, sizeof(dest_sockaddr));
                if (sent < 0) {
                    perror("sendto destination");
                }
            }
        }

        // Check UDP destination socket (data from destination)
        if (FD_ISSET(udp_dest_sockfd, &readfds)) {
            from_len = sizeof(from_addr);
            n = recvfrom(udp_dest_sockfd, buffer, BUFFER_SIZE, 0,
                        (struct sockaddr *)&from_addr, &from_len);
            if (n > 0) {
                char from_str[INET6_ADDRSTRLEN];
                inet_ntop(AF_INET6, &from_addr.sin6_addr, from_str, INET6_ADDRSTRLEN);
                // printf("Child %d: Received %zd bytes from destination [%s]:%d, forwarding to client\n", session_idx, n, from_str, ntohs(from_addr.sin6_port));

                // Forward back to client
                struct sockaddr_in6 client_dest;
                memset(&client_dest, 0, sizeof(client_dest));
                client_dest.sin6_family = AF_INET6;
                memcpy(&client_dest.sin6_addr, &forwardtab[session_idx].sourceaddr, sizeof(struct in6_addr));
                client_dest.sin6_port = htons(forwardtab[session_idx].sourcept);
                        
                ssize_t sent = sendto(udp_client_sockfd, buffer, n, 0,
                        (struct sockaddr *)&client_dest, sizeof(client_dest));
                if (sent < 0) {
                    perror("sendto client");
                }
                // } else {
                //     printf("Child %d: Forwarded %zd bytes to client\n", session_idx, sent);
                // }
            }
        }
    }
    printf("Child %d: Exiting\n", session_idx);
    exit(EXIT_SUCCESS);
}
