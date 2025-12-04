#define _POSIX_C_SOURCE 200809L

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include "setup_udp_client.h"
#include "send_ping_requests.h"
#include "calculate_statistics.h"
#include "install_alarm_handler_udp.h"

int main(int argc, char *argv[]) {
    if (argc != 6) {
        fprintf(stderr, "Usage: %s <servip> <portnum> <secret> <portnum2> <pcount>\n", argv[0]);
        exit(1);
    }

    char *server_ip = argv[1];
    int server_port = atoi(argv[2]);
    char secret[7];
    strncpy(secret, argv[3], 6);
    secret[6] = '\0';
    int client_port = atoi(argv[4]);
    int pcount = atoi(argv[5]);

    if (strlen(argv[3]) != 6) {
        fprintf(stderr, "Error: secret must be exactly 6 characters\n");
        exit(1);
    }

    if (pcount > 100) {
        fprintf(stderr, "Error: pcount should not exceed 100\n");
        exit(1);
    }

    if (install_alarm_handler() == -1) {
        perror("sigaction");
        exit(1);
    }

    int sockfd = setup_udp_client(server_ip, client_port);
    if (sockfd == -1) {
        fprintf(stderr, "Failed to setup UDP client.\n");
        exit(1);
    }

    // Seed random number generator and generate initial sequence number
    srand(time(NULL));
    unsigned int initial_seq = rand() % 1000;

    double *rtt_array = (double *)calloc(pcount, sizeof(double));
    if (rtt_array == NULL) {
        perror("calloc");
        close(sockfd);
        exit(1);
    }

    printf("Starting UDP ping to %s:%d\n", server_ip, server_port);

    set_server_address(server_ip, server_port);
    send_ping_requests(sockfd, secret, initial_seq, pcount, rtt_array);

    calculate_statistics(rtt_array, pcount);

    free(rtt_array);
    close(sockfd);
    return 0;
}