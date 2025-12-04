#include "file_io.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static uint32_t loss_seqs[10]; //store up to 10 loss sequences
static int loss_count = 0;
static uint8_t dropped[10] = {0}; //see if you have already dropped this seq

void read_sender_loss_model() {
    FILE *fp = fopen("sender.lossmodel", "r");
    if (!fp) return;
    
    loss_count = 0;
    while (loss_count < 10 && fscanf(fp, "%u", &loss_seqs[loss_count]) == 1) {
        loss_count++;
    }
    fclose(fp);
}

int should_drop_ack(uint32_t seq) {
    for (int i = 0; i < loss_count; i++) {
        if (loss_seqs[i] == seq && !dropped[i]) {
            dropped[i] = 1;
            return 1;
        }
    }
    return 0;
}

char* read_file_to_memory(const char *filename, uint32_t *filesize) {
    FILE *fp = fopen(filename, "rb");
    if (!fp) {
        perror("fopen");
        return NULL;
    }
    
    //getting file size by seeking to end and telling position
    fseek(fp, 0, SEEK_END);
    *filesize = ftell(fp);
    fseek(fp, 0, SEEK_SET);
    
    char *buffer = malloc(*filesize);
    if (!buffer) {
        fclose(fp);
        return NULL;
    }
    
    fread(buffer, 1, *filesize, fp);
    fclose(fp);
    
    return buffer;
}