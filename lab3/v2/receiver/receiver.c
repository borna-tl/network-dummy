#include "receiver.h"
#include "file_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <sys/time.h>

void init_receiver_state(ReceiverState *state) {
    memset(state, 0, sizeof(*state));
}

void handle_init_packet(int sockfd, ReceiverState *state, char *packet, struct sockaddr_in *from) {
    // extract init packet data
    strncpy(state->filename, packet, 6);
    state->filename[6] = '\0';
    state->filesize = ntohl(*(uint32_t*)(packet + 8)); // corrected offset because of padding
    state->payloadsize = ntohl(*(uint32_t*)(packet + 12)); // corrected offset because of padding

    state->numpackets = (state->filesize + state->payloadsize - 1) / state->payloadsize; //calc number of packets needed
    
    state->received = calloc(state->numpackets, sizeof(uint8_t));
    state->filedata = malloc(state->filesize);
    state->next_expected = 0;
    state->transfer_active = 1;
    memcpy(&state->sender_addr, from, sizeof(*from));
    
    // Send ACK (filename)
    sendto(sockfd, state->filename, 6, 0, (struct sockaddr*)from, sizeof(*from));

    // Set up 250 ms timeout for waiting for first data packet
    struct timeval tv;
    tv.tv_sec = 0;
    tv.tv_usec = 250000;
    setsockopt(sockfd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
}

void send_ack(int sockfd, struct sockaddr_in *dest, uint32_t seq) {
    uint32_t ack = htonl(seq);
    sendto(sockfd, &ack, 4, 0, (struct sockaddr*)dest, sizeof(*dest));
}

void handle_data_packet(int sockfd, ReceiverState *state, char *packet, ssize_t len, struct sockaddr_in *from) {
    if (len < 4) 
        return;
    
    uint32_t seq = ntohl(*(uint32_t*)packet);
    
    // Check loss model
    if (should_drop_data(seq)) {
        printf("dropping seq %u...\n", seq);
        return;
    }
    
    if (seq < state->numpackets && !state->received[seq]) {
        // Copy data
        uint32_t offset = seq * state->payloadsize;
        uint32_t data_len = len - 4;

        memcpy(state->filedata + offset, packet + 4, data_len);
        state->received[seq] = 1;
        
        // Update next_expected to point to next missing packet
        while (state->next_expected < state->numpackets && 
               state->received[state->next_expected]) {
            state->next_expected++;
        }
    }
    
    // Send cumulative ACK
    send_ack(sockfd, from, state->next_expected);
    
    // If all packets received, finalize
    if (state->next_expected == state->numpackets) {
        // Send final ACKs
        for (int i = 0; i < 10; i++) {
            send_ack(sockfd, from, state->next_expected);
            usleep(10000); // 10ms spacing
        }
        
        // Write file
        write_file(state->filename, state->filedata, state->filesize);
        printf("File %s_n received successfully.\n", state->filename);
        
        // Reset for next transfer
        free(state->received);
        free(state->filedata);
        init_receiver_state(state);
    }
}