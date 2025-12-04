#ifndef FILE_IO_H
#define FILE_IO_H

#include <stdint.h>

char* read_file_to_memory(const char *filename, uint32_t *filesize);
int should_drop_ack(uint32_t seq);
void read_sender_loss_model();

#endif