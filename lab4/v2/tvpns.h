#ifndef TVPNS_H
#define TVPNS_H
#include "server_child.h"
#include "utilities.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/socket.h>
#include <sys/select.h>
#include <errno.h>
#include <signal.h>

#define NUMSESSIONS 8
#define BASE_UDP_PORT 55500
#define BASE_TUNNEL_PORT 57500
#define MAX_BIND_ATTEMPTS 100
#define BUFFER_SIZE 1024

struct tunneltab {
    unsigned long destaddr;
    unsigned short destpt;
    unsigned long sourceaddr;
    unsigned short sourcept;
    unsigned short tunnelpt;
};

extern struct tunneltab forwardtab[NUMSESSIONS];

#endif
