#include "file_io.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static uint32_t loss_seqs[10];
static int loss_count = 0;
static uint8_t dropped[10] = {0};

void read_receiver_loss_model() {
    FILE *fp = fopen("receiver.lossmodel", "r");
    if (!fp) return;
    
    loss_count = 0;
    while (loss_count < 10 && fscanf(fp, "%u", &loss_seqs[loss_count]) == 1) {
        loss_count++;
    }
    fclose(fp);
}

int should_drop_data(uint32_t seq) {
    for (int i = 0; i < loss_count; i++) {
        if (loss_seqs[i] == seq && !dropped[i]) {
            dropped[i] = 1;
            return 1;
        }
    }
    return 0;
}

void write_file(const char *filename, const char *data, uint32_t size) {
    char outname[64];
    snprintf(outname, sizeof(outname), "%s_n", filename);
   
    FILE *fp = fopen(outname, "wb");
    if (!fp) {
        perror("fopen");
        return;
    }
    
    fwrite(data, 1, size, fp);
    fclose(fp);
}