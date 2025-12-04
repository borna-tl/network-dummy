#define _POSIX_C_SOURCE 200809L
#include "execute_command.h"
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/wait.h>

void execute_command(char *cmd, int client_fd)
{
    printf("Executing command '%s'\n", cmd);
    fflush(stdout);
    sleep(1);

    pid_t pid = fork();
    if (pid < 0) { 
        perror("fork");
    } else if (pid == 0) {
        // child - redirect stdout to client socket
        if (dup2(client_fd, 1) == -1) {
            perror("dup2");
            exit(1);
        }
        close(client_fd);
        
        char *argv_exec[2] = { cmd, NULL };
        if (execvp(argv_exec[0], argv_exec) == -1){
            fprintf(stderr, "exec '%s' failed: %s\n", cmd, strerror(errno));
            fflush(stderr);
            exit(1);
        }
    } else {
        // parent
        int st;
        waitpid(pid, &st, 0);
    }
}