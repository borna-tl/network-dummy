#include "sender.h"
#include "file_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h> 
#include <arpa/inet.h>


void read_sender_params(SenderParams *params) {
    FILE *fp = fopen("sender.param", "r");
    if (!fp) {
        perror("sender.param");
        exit(1);
    }
    
    fscanf(fp, "%u", &params->maxfilesize);
    fscanf(fp, "%u", &params->micropace);
    fscanf(fp, "%u", &params->payloadsize);
    fclose(fp);
}

int main(int argc, char *argv[]) {
    if (argc != 4) {
        fprintf(stderr, "Usage: %s <rcvip> <rcvport> <filename>\n", argv[0]);
        return 1;
    }
    
    // Read parameters
    SenderParams params;
    read_sender_params(&params);
    
    // Validate filename
    if (strlen(argv[3]) != 6) {
        fprintf(stderr, "Error: Filename must be exactly 6 characters\n");
        return 1;
    }
    
    // Read file
    uint32_t filesize;
    char *filedata = read_file_to_memory(argv[3], &filesize);
    if (!filedata) {
        fprintf(stderr, "Error: Cannot read file\n");
        return 1;
    }
    
    if (filesize > params.maxfilesize) {
        fprintf(stderr, "Error: File size %u exceeds maximum %u\n", 
                filesize, params.maxfilesize);
        free(filedata);
        return 1;
    }
    
    // Get sender loss model
    read_sender_loss_model();
    
    // Setup socket
    int sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    struct sockaddr_in dest;
    memset(&dest, 0, sizeof(dest));
    dest.sin_family = AF_INET;
    dest.sin_port = htons( atoi(argv[2]));
    inet_pton(AF_INET, argv[1], &dest.sin_addr);
    
    // Setup signal handlers
    setup_signal_handlers();
    
    // Send init packet
    send_init_packet(sockfd, &dest, argv[3], filesize, params.payloadsize);
    
    // Send data
    send_data_segments(sockfd, &dest, filedata, filesize, &params);
    
    free(filedata);
    return 0;
}