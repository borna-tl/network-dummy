#ifndef RECV_FILE_IO_H
#define RECV_FILE_IO_H

#include <stdint.h>

void write_file(const char *filename, const char *data, uint32_t size);
int should_drop_data(uint32_t seq);
void read_receiver_loss_model();

#endif