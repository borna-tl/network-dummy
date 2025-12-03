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
#include <net/if.h>
#include <ifaddrs.h>

int setup_udp_client(const char *server_ip, int client_port) {
    int sockfd = socket(AF_INET6, SOCK_DGRAM, 0);
    if (sockfd == -1) {
        perror("socket");
        return -1;
    }

    // Get the scope ID for eth0 using if_nametoindex()
    unsigned int scope_id = if_nametoindex("eth0");
    if (scope_id == 0) {
        fprintf(stderr, "Error: Could not get scope ID for eth0\n");
        close(sockfd);
        return -1;
    }

    // Get the IPv6 address for eth0
    struct ifaddrs *ifaddr, *ifa;
    if (getifaddrs(&ifaddr) == -1) {
        perror("getifaddrs");
        close(sockfd);
        return -1;
    }

    char ipv6_addr[INET6_ADDRSTRLEN];
    int found = 0;
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
        close(sockfd);
        return -1;
    }

    struct sockaddr_in6 client_addr;
    memset(&client_addr, 0, sizeof(client_addr));
    client_addr.sin6_family = AF_INET6;
    client_addr.sin6_port = htons(client_port);
    client_addr.sin6_scope_id = scope_id;
    
    if (inet_pton(AF_INET6, ipv6_addr, &client_addr.sin6_addr) != 1) {
        fprintf(stderr, "Error: Invalid IPv6 address for client\n");
        close(sockfd);
        return -1;
    }

    printf("Client binding to [%s%%eth0]:%d (scope_id=%u)\n", ipv6_addr, client_port, scope_id);

    if (bind(sockfd, (struct sockaddr *)&client_addr, sizeof(client_addr)) == -1) {
        fprintf(stderr, "Error: Could not bind to %d\n", client_port);        
        close(sockfd);
        exit(1);
    }

    return sockfd;
}