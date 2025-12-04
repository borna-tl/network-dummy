#define _POSIX_C_SOURCE 200809L
#define _DEFAULT_SOURCE
#include "sender.h"
#include "alarm_handler.h"
#include "file_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <signal.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>

volatile uint8_t *ack_received = NULL;
volatile uint32_t next_expected = 0;
volatile int numpackets = 0;

static int sockfd;

void sigio_handler(int sig) {
    char buffer[16];
    ssize_t n = recv(sockfd, buffer, sizeof(buffer), MSG_DONTWAIT);
    //if we are sending a packet, get ack for last received packet (this points to next expected)
    if (n >= 4) {
        uint32_t ack_seq = ntohl(*(uint32_t*)buffer);
        
        // Check loss model
        if (should_drop_ack(ack_seq)) {
            return;
        }
        
        // If ACK equals numpackets, all done.
        if (ack_seq >= numpackets) { 
            for (uint32_t i = 0; i < numpackets; i++) {
                ack_received[i] = 1;
            }
            next_expected = numpackets;
            return;
        }
        
        // Update next_expected and mark all segments < ack_seq as received
        if (ack_seq > next_expected) {
            for (uint32_t i = next_expected; i < ack_seq && i < numpackets; i++) {
                ack_received[i] = 1;
            }
            next_expected = ack_seq;
        }
    }
}

void setup_signal_handlers(void) {
    struct sigaction sa_io;
    
    setup_alarm_handler();
    
    sa_io.sa_handler = sigio_handler;
    sigemptyset(&sa_io.sa_mask);
    sa_io.sa_flags = 0;
    sigaction(SIGIO, &sa_io, NULL);
}

void send_init_packet(int sock, struct sockaddr_in *dest, const char *filename, uint32_t filesize, uint32_t payloadsize) {
    InitPacket pkt;
    memset(&pkt, 0, sizeof(pkt));
    strncpy(pkt.filename, filename, 6);
    pkt.filesize = htonl(filesize);
    pkt.payloadsize = htonl(payloadsize);
    
    char response[16];
    int attempts = 0;
    
    while (attempts < MAX_PARAM_RETRIES) { // if we haven't received ack after 5 retries, exit
        if (sendto(sock, &pkt, sizeof(pkt), 0, (struct sockaddr*)dest, sizeof(*dest)) == -1){
            perror("sendto");
            exit(1);
        }
        
        start_alarm_timer(PARAM_TIMEOUT_MS);
        
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);

        // Set timeout using setsockopt instead of signals
        struct timeval tv;
        tv.tv_sec = 0;
        tv.tv_usec = PARAM_TIMEOUT_MS * 1000;
        setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));

        //this is an ack from receiver that it got the init packet
        ssize_t n = recvfrom(sock, response, sizeof(response), 0, (struct sockaddr*)&from, &fromlen);
        //check that response matches filename
        if (n >= 6 && memcmp(response, filename, 6) == 0) {
            stop_alarm_timer();
            return;
        }
        if (n == -1){
            perror("recvfrom");
        }
        attempts++;
    }
    
    fprintf(stderr, "Error: Failed to establish connection after %d attempts\n", 
            MAX_PARAM_RETRIES);
    exit(1);
}

int send_data_segments(int sock, struct sockaddr_in *dest, 
                       const char *filedata, uint32_t filesize, 
                       const SenderParams *params) {
    sockfd = sock;
    numpackets = (filesize + params->payloadsize - 1) / params->payloadsize;
    
    // Allocate tracking array and retry count array
    uint8_t *track = calloc(numpackets, sizeof(uint8_t));
    uint8_t *retry_count = calloc(numpackets, sizeof(uint8_t));
    ack_received = track;
    next_expected = 0;
    
    // Enable async IO
    fcntl(sock, F_SETOWN, getpid());
    int flags = fcntl(sock, F_GETFL);
    fcntl(sock, F_SETFL, flags | O_ASYNC);
    
    struct timeval start, end;
    gettimeofday(&start, NULL);
    
    int all_acked = 0; //have we received acks for all packets?
    while (!all_acked) {
        all_acked = 1;
        
        for (uint32_t i = 0; i < numpackets; i++) {
            if (!ack_received[i]) { //packet i has not been acked
                all_acked = 0;
                
                // check retry limit
                if (retry_count[i] >= MAX_DATA_RETRIES) { 
                    fprintf(stderr, "Error: Packet %u failed after %d retransmission attempts\n", 
                            i, MAX_DATA_RETRIES);
                    free(track);
                    free(retry_count);
                    exit(1);
                }
                
                // build packets using appropriate offset and length
                uint32_t offset = i * params->payloadsize;
                //note that length is a constant unless we are at the end of the file
                uint32_t len = (offset + params->payloadsize > filesize) ? 
                               (filesize - offset) : params->payloadsize;
                
                char *packet = malloc(4 + len);
                *(uint32_t*)packet = htonl(i);
                memcpy(packet + 4, filedata + offset, len);
                
                sendto(sock, packet, 4 + len, 0, 
                       (struct sockaddr*)dest, sizeof(*dest));
                free(packet);
                
                retry_count[i]++; // Increment retry count for packet i
                
                usleep(params->micropace); //wait 20ms before sending next packet
            }
        }
    }
    
    gettimeofday(&end, NULL);
    long elapsed = (end.tv_sec - start.tv_sec) * 1000 + 
                   (end.tv_usec - start.tv_usec) / 1000;
    
    printf("File transfer completed in %ld ms\n", elapsed);
    
    free(track);
    free(retry_count);
    return 0;
}