#define _POSIX_C_SOURCE 200809L
#include "calculate_statistics.h"
#include <stdio.h>
#include <math.h>

void calculate_statistics(const double *rtt_array, int pcount) {
    int received = 0;
    double sum = 0.0;
    double min_rtt = 0.0;
    double max_rtt = 0.0;

    for (int i = 0; i < pcount; i++) {
        if (rtt_array[i] > 0.0) {
            received++;
            sum += rtt_array[i];
            if (min_rtt == 0.0 || rtt_array[i] < min_rtt) {
                min_rtt = rtt_array[i];
            }
            if (rtt_array[i] > max_rtt) {
                max_rtt = rtt_array[i];
            }
        }
    }

    double avg_rtt = 0.0;
    double std_dev = 0.0;

    if (received > 0) {
        avg_rtt = sum / received;

        double variance_sum = 0.0;
        for (int i = 0; i < pcount; i++) {
            if (rtt_array[i] > 0.0) {
                double diff = rtt_array[i] - avg_rtt;
                variance_sum += diff * diff;
            }
        }
        std_dev = sqrt(variance_sum / received);
    }

    printf("\n--- Ping Statistics ---\n");
    printf("Packets sent: %d\n", pcount);
    printf("Packets received: %d\n", received);
    printf("Packet loss: %.1f%%\n", ((pcount - received) * 100.0) / pcount);
    
    if (received > 0) {
        printf("RTT Avg = %.2f us\n", avg_rtt);
        printf("RTT Min = %.2f us\n", min_rtt);
        printf("RTT Max = %.2f us\n", max_rtt);
        printf("RTT STD = %.2f us\n", std_dev);
        printf("--- Ping Statistics ---\n");
    }
}