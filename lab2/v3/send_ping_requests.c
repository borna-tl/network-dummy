#define _POSIX_C_SOURCE 200809L
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

#define PACKET_SIZE 10

static struct sockaddr_in server_addr;

void set_server_address(const char *server_ip, int server_port) {
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = inet_addr(server_ip);
    server_addr.sin_port = htons(server_port);
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
        
        // Set timer using setitimer for 1.234567 seconds
        // note that ualarm was deprecated, so we looked for alternative
        struct itimerval timer;
        timer.it_value.tv_sec = 1;
        timer.it_value.tv_usec = 234567;
        timer.it_interval.tv_sec = 0;
        timer.it_interval.tv_usec = 0;
        setitimer(ITIMER_REAL, &timer, NULL);

        ssize_t sent = sendto(sockfd, packet, PACKET_SIZE, 0,
                             (struct sockaddr *)&server_addr, sizeof(server_addr));
        if (sent == -1) {
            perror("sendto");
            struct itimerval stop_timer = {0};
            setitimer(ITIMER_REAL, &stop_timer, NULL);
            seq_num++;
            continue;
        }
        char response[PACKET_SIZE];
        struct sockaddr_in from_addr;
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
            
            // Cancel timer
            struct itimerval stop_timer = {0};
            setitimer(ITIMER_REAL, &stop_timer, NULL);

            if (n == PACKET_SIZE) {
                if (strncmp(response, secret, 6) == 0) {
                    unsigned int recv_seq;
                    memcpy(&recv_seq, response + 6, 4);
                    recv_seq = ntohl(recv_seq); //convert from network byte order to host byte order

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