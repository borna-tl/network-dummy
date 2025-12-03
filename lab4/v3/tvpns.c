#include "tvpns.h"

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <server_ip> <port> <secret>\n", argv[0]);
        exit(EXIT_FAILURE);
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    char *secret = argv[3];

    if (strlen(secret) != 6) {
        fprintf(stderr, "Error: secret must be a string of 6 ASCII upper and lower case characters.\n");
        exit(EXIT_FAILURE);
    }

    initialize_forward_table();

    // Ignore SIGCHLD to prevent zombie processes
    signal(SIGCHLD, SIG_IGN);

    // Create TCP socket to listen for connections
    int tcp_sockfd = socket(AF_INET6, SOCK_STREAM, 0);
    if (tcp_sockfd == -1) {
        perror("socket");
        exit(EXIT_FAILURE);
    }

    // // Set socket options
    // int opt = 1;
    // if (setsockopt(tcp_sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
    //     perror("setsockopt");
    //     close(tcp_sockfd);
    //     exit(EXIT_FAILURE);
    // }

    // Get the scope ID for eth0 using if_nametoindex()
    unsigned int scope_id = if_nametoindex("eth0");
    if (scope_id == 0) {
        fprintf(stderr, "Error: Could not get scope ID for eth0\n");
        close(tcp_sockfd);
        return -1;
    }

    // Get the IPv6 address for eth0
    struct ifaddrs *ifaddr, *ifa;
    char ipv6_addr[INET6_ADDRSTRLEN];
    int found = 0;

    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        close(tcp_sockfd);
        return -1;
    }

    // Search for eth0's link-local IPv6 address
    for (ifa = ifaddr; ifa != NULL; ifa = ifa->ifa_next) {
        if (ifa->ifa_addr == NULL)
            continue;

        if (strcmp(ifa->ifa_name, "eth0") == 0 && 
            ifa->ifa_addr->sa_family == AF_INET6) {
            struct sockaddr_in6 *addr6 = (struct sockaddr_in6 *)ifa->ifa_addr;
            
            // Check if it's a link-local address (i.e., starts with fe80::)
            if (IN6_IS_ADDR_LINKLOCAL(&addr6->sin6_addr)) {
                inet_ntop(AF_INET6, &addr6->sin6_addr, ipv6_addr, INET6_ADDRSTRLEN);
                found = 1;
                break;
            }
        }
    }

    freeifaddrs(ifaddr);

    if (!found) {
        fprintf(stderr, "Error: Could not find link-local IPv6 address for eth0\n");
        close(tcp_sockfd);
        return -1;
    }

    struct sockaddr_in6 server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(server_port);
    server_addr.sin6_scope_id = scope_id;
    
    if (inet_pton(AF_INET6, ipv6_addr, &server_addr.sin6_addr) != 1) {
        fprintf(stderr, "Error: Invalid IPv6 address for server\n");
        close(tcp_sockfd);
        return -1;
    }

    printf("Server binding to [%s%%eth0]:%d (scope_id=%u)\n", ipv6_addr, server_port, scope_id);

    // Bind TCP socket
    if (bind(tcp_sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr)) == -1) {
        perror("bind");
        close(tcp_sockfd);
        return -1;
    }

    // Listen for connections
    if (listen(tcp_sockfd, 5) == -1) {
        perror("listen");
        close(tcp_sockfd);
        exit(EXIT_FAILURE);
    }
    printf("Server listening on %s:%d\n", server_ip, server_port);
    // Accept connections
    while (1) {
        struct sockaddr_in6 client_addr;
        socklen_t client_len = sizeof(client_addr);
        
        // Accept a new TCP connection
        int client_sockfd = accept(tcp_sockfd, (struct sockaddr *)&client_addr, &client_len);
        if (client_sockfd == -1) {
            perror("accept");
            continue;
        }

        // Read connection request (2 bytes: magic value 315)
        uint16_t conn_req;
        ssize_t n = read(client_sockfd, &conn_req, 2);
        if (n != 2 || ntohs(conn_req) != 315) {
            printf("Invalid connection request (got %d, expected 315)\n", ntohs(conn_req));
            close(client_sockfd);
            continue;
        }

        // Read secret key (6 bytes)
        char recv_secret[7];
        n = read(client_sockfd, recv_secret, 6);
        if (n != 6) {
            printf("Failed to read secret key\n");
            close(client_sockfd);
            continue;
        }
        recv_secret[6] = '\0';

        if (strncmp(recv_secret, secret, 6) != 0) {
            printf("Invalid secret key: got '%s', expected '%s'\n", recv_secret, secret);
            close(client_sockfd);
            continue;
        }

        // Read destination IPv6 address (16 bytes)
        struct in6_addr dest_addr;
        n = read(client_sockfd, &dest_addr.s6_addr, 16);
        if (n != 16) {
            printf("Failed to read destination address\n");
            close(client_sockfd);
            continue;
        }

        // Read destination port (2 bytes)
        uint16_t dest_port;
        n = read(client_sockfd, &dest_port, 2);
        if (n != 2) {
            printf("Failed to read destination port\n");
            close(client_sockfd);
            continue;
        }

        // Read source IPv6 address (16 bytes)
        struct in6_addr source_addr;
        n = read(client_sockfd, &source_addr.s6_addr, 16);
        if (n != 16) {
            printf("Failed to read source address\n");
            close(client_sockfd);
            continue;
        }

        // Use separate buffers for inet_ntoa to avoid overwriting
        char dest_ip_str[INET6_ADDRSTRLEN];
        char src_ip_str[INET6_ADDRSTRLEN];
        inet_ntop(AF_INET6, &dest_addr, dest_ip_str, INET6_ADDRSTRLEN);
        inet_ntop(AF_INET6, &source_addr, src_ip_str, INET6_ADDRSTRLEN);

        // Find free session slot
        int session_idx = find_free_session_slot();
        if (session_idx == -1) {
            printf("No available session slots\n");
            close(client_sockfd);
            continue;
        }

        // Update forwarding table
        memcpy(&forwardtab[session_idx].destaddr, &dest_addr, sizeof(struct in6_addr));
        forwardtab[session_idx].destpt = ntohs(dest_port);
        memcpy(&forwardtab[session_idx].sourceaddr, &source_addr, sizeof(struct in6_addr));
        forwardtab[session_idx].sourcept = 0; // Will be set when first packet arrives

        printf("New session %d: src=[%s] dst=[%s]:%d\n", session_idx, src_ip_str, dest_ip_str, ntohs(dest_port));

        // Fork child process
        pid_t pid = fork();
        if (pid == -1) {
            perror("fork");
            close(client_sockfd);
            continue;
        }

        if (pid == 0){
            close(tcp_sockfd); // Child doesn't need listener socket
            run_child_session(session_idx, client_sockfd, secret);
        } else {
            // Parent process
            close(client_sockfd);
        }
    }
    printf("Server shutting down.\n");
    close(tcp_sockfd);
    return 0;
}

