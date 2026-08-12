#define _POSIX_C_SOURCE 200809L
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client_inet.h"
#include "client_cli.h"
#include "client.h"

volatile sig_atomic_t quitSignal = 1;

void handleSignal(int sig) {
    (void)sig;
    quitSignal = 0;
}

int main(int argc, char *argv[]) {

    if (argc != 3) {
        printf("Use ./client <ip> <port>\n");
        exit(EXIT_FAILURE);
    }

    // SIGNAL
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    char buffer[256];
    int fdServer = serverConnect(argc, argv);

    if (clientAuth(fdServer) == -1) {
        perror("authentication error");
        exit(EXIT_FAILURE);
    }

    while (quitSignal) {
        quitSignal = cliRun(fdServer, buffer, 256);
    }

    serverDisconnect(fdServer);
    exit(EXIT_SUCCESS);
}
