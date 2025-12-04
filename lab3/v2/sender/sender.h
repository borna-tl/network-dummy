#ifndef SENDER_H
#define SENDER_H

#include <stdint.h>
#include <netinet/in.h>
#include <sys/socket.h>

#define MAX_PAYLOAD_SIZE 1400
#define PARAM_TIMEOUT_MS 250
#define MAX_PARAM_RETRIES 5
#define MAX_DATA_RETRIES 10

typedef struct {
    uint32_t maxfilesize;
    uint32_t micropace;
    uint32_t payloadsize;
} SenderParams;

typedef struct {
    char filename[7]; // 7 bytes for filename + 1 byte padding 
    uint32_t filesize; //starts from byte 8
    uint32_t payloadsize; //starts from byte 12
} InitPacket;

// Volatile for signal handler access
extern volatile uint8_t *ack_received;
extern volatile uint32_t next_expected;
extern volatile int numpackets;

void read_sender_params(SenderParams *params);
void setup_signal_handlers(void);
void send_init_packet(int sockfd, struct sockaddr_in *dest, 
                      const char *filename, uint32_t filesize, uint32_t payloadsize);
int send_data_segments(int sockfd, struct sockaddr_in *dest, 
                       const char *filedata, uint32_t filesize, 
                       const SenderParams *params);

#endif