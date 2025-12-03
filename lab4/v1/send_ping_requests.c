#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "send_ping_requests.h"
#include "alarm_handler_udp.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <sys/time.h>
#include <errno.h>
#include <unistd.h>

#define PACKET_SIZE 10

static struct sockaddr_in6 server_addr;

void set_server_address(const char *server_ip, int server_port) {
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin6_family = AF_INET6;
    server_addr.sin6_port = htons(server_port);
    server_addr.sin6_scope_id = 0;
    
    if (inet_pton(AF_INET6, server_ip, &server_addr.sin6_addr) != 1) {
        fprintf(stderr, "Error: Invalid IPv6 server address\n");
    }
}

void send_ping_requests(int sockfd, const char *secret, unsigned int initial_seq, 
                       int pcount, double *rtt_array) {
    unsigned int seq_num = initial_seq;
    
    for (int i = 0; i < pcount; i++) {
        char packet[PACKET_SIZE];
        memcpy(packet, secret, 6);
        unsigned int seq_network = htonl(seq_num);
        memcpy(packet + 6, &seq_network, 4);

        struct timeval send_time, recv_time;
        gettimeofday(&send_time, NULL);

        alarm_sent_off = 0;
        
        // Set timer using ualarm for 1.234567 seconds (1234567 microseconds)
        ualarm(1234567, 0);
        ssize_t sent = sendto(sockfd, packet, PACKET_SIZE, 0,
                             (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sent == -1) {
            perror("sendto");
            ualarm(0, 0);
            seq_num++;
            continue;
        }
        char response[PACKET_SIZE];
        struct sockaddr_in6 from_addr;
        socklen_t from_len = sizeof(from_addr);

        while (!alarm_sent_off) {
            ssize_t n = recvfrom(sockfd, response, PACKET_SIZE, 0,
                                (struct sockaddr *)&from_addr, &from_len);
            
            if (n == -1) {
                if (errno == EINTR && alarm_sent_off) {
                    break;
                }
                continue;
            }

            gettimeofday(&recv_time, NULL);
            
            ualarm(0, 0);

            if (n == PACKET_SIZE) {
                if (strncmp(response, secret, 6) == 0) {
                    unsigned int recv_seq;
                    memcpy(&recv_seq, response + 6, 4);
                    recv_seq = ntohl(recv_seq); // Convert from network byte order to host byte order

                    if (recv_seq == seq_num) {
                        double rtt = (recv_time.tv_sec - send_time.tv_sec) * 1000000.0 +
                                    (recv_time.tv_usec - send_time.tv_usec);
                        rtt_array[i] = rtt;
                        printf("Ping %d: seq=%u, RTT=%.2f us\n", i+1, seq_num, rtt);
                        break;
                    }
                }
            }
        }

        if (alarm_sent_off) {
            printf("Ping %d: seq=%u, timeout\n", i+1, seq_num);
            rtt_array[i] = 0.0;
        }

        seq_num++;
    }
}