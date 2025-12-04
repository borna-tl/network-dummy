#include "receiver.h"
#include "file_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>


int main(int argc, char *argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <port>\n", argv[0]);
        return 1;
    }
    
    int port = atoi(argv[1]);
    
    // read loss model
    read_receiver_loss_model();
    
    // create socket and set up port
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;
    bind(sockfd, (struct sockaddr*)&addr, sizeof(addr));
    
    printf("Receiver listening on port %d\n", port);
    
    ReceiverState state;
    init_receiver_state(&state);

    int init_retry_count = 0; //counts retries for init packet
    char buffer[2048];
    while (1) {
        struct sockaddr_in from;
        socklen_t fromlen = sizeof(from);
        
        ssize_t n = recvfrom(sockfd, buffer, sizeof(buffer), 0,
                             (struct sockaddr*)&from, &fromlen);
        
        // handle timeout during init phase
        if (n < 0 && errno == EAGAIN && state.transfer_active && init_retry_count > 0) {
            if (init_retry_count >= 5) { //if we have retried 5 times (each time we wait 250ms), we will reset and wait for a new init packet
                printf("No data received after 5 init retries; resetting...\n");
                free(state.received);
                free(state.filedata);
                init_receiver_state(&state);
                init_retry_count = 0;
                
                // remove timer
                struct timeval tv = {0, 0};
                setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                continue;
            }
            
            // Retransmit init ACK
            sendto(sockfd, state.filename, 6, 0, (struct sockaddr*)&state.sender_addr, sizeof(state.sender_addr));
            init_retry_count++;
            continue;
        }
        if (n == -1){
            perror("recvfrom");
            continue;
        }
        if (n <= 0) {
            printf("Connection closed or error\n");
            continue;
        }
        // Check if init packet (16 bytes)
        if (n >= 14 && n <= 16 && !state.transfer_active) {
            handle_init_packet(sockfd, &state, buffer, &from);
            init_retry_count = 1; 
        }
        // Duplicate init during active transfer
        else if (n >= 14 && n <= 16 && state.transfer_active) {
            // Re-send ACK
            sendto(sockfd, state.filename, 6, 0, (struct sockaddr*)&from, sizeof(from));
        }
        // data packets
        else if (state.transfer_active) {
            // first data packet received so we remove timer
            if (init_retry_count > 0) {
                struct timeval tv = {0, 0};
                setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
                init_retry_count = 0;
            }
            handle_data_packet(sockfd, &state, buffer, n, &from);
        }
        else{
            printf("Ignoring unexpected packet of size %zd\n", n);
        }
    }
    
    return 0;
}