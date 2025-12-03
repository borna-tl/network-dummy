#ifndef SEND_PING_REQUESTS_H
#define SEND_PING_REQUESTS_H

void set_server_address(const char *server_ip, int server_port);
void send_ping_requests(int sockfd, const char *secret, unsigned int initial_seq, 
                       int pcount, double *rtt_array);

#endif