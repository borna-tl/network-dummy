#ifndef RECEIVER_H
#define RECEIVER_H

#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>

typedef struct {
    char filename[7];
    uint32_t filesize;
    uint32_t payloadsize;
    uint32_t numpackets;
    uint8_t *received;
    char *filedata;
    uint32_t next_expected;
    int transfer_active;
    struct sockaddr_in sender_addr;
} ReceiverState;

void init_receiver_state(ReceiverState *state);
void handle_init_packet(int sockfd, ReceiverState *state, 
                        char *packet, struct sockaddr_in *from);
void handle_data_packet(int sockfd, ReceiverState *state, 
                        char *packet, ssize_t len, struct sockaddr_in *from);
void send_ack(int sockfd, struct sockaddr_in *dest, uint32_t seq);

#endif