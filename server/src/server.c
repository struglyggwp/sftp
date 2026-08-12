#define _POSIX_C_SOURCE 200809L // sigaction без него не определяется
#include <stdlib.h>
#include <stdio.h>
#include <pthread.h>
#include <unistd.h>
#include <signal.h>
#include <errno.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include "../include/log.h"
#include "../include/client_handler.h"

#define MAX_CLIENTS 64
#define SERVER_PORT 9999
#define LOG_FILE_PATH "srv/log.txt"

volatile sig_atomic_t keepRunning = 1;

void handleSignal(int sig) {
    (void)sig;
    keepRunning = 0;
}

int initServer(int port) {

    int serverFd = socket(AF_INET, SOCK_STREAM, 0);
    if (serverFd < 0) {
        writeLog(logError, "Server socket error", errno);
        return -1;
    }

    // Для дебага
    int opt = 1;
    if (setsockopt(serverFd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        close(serverFd);
        writeLog(logError, "Server setsockopt error", errno);
        return -1;
    }

    struct sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = INADDR_ANY;

    if (bind(serverFd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(serverFd);
        writeLog(logError, "Server bind error", errno);
        return -1;
    }

    if (listen(serverFd, MAX_CLIENTS) < 0) {
        close(serverFd);
        writeLog(logError, "Server listen error", errno);
        return -1;
    }

    return serverFd;
}

int runClientHandler(int clientFd) {
    // Передадим в поток указатель, от греха подальше
    pthread_t threadId;
    int *clientFdPtr = malloc(sizeof(int));
    if (clientFdPtr == NULL) {
        close(clientFd);
        writeLog(logError, "Failed to allocate memory for client handler", errno);
        return -1;
    }
    *clientFdPtr = clientFd;

    if (pthread_create(&threadId, NULL, clientHandler, clientFdPtr) != 0) {
        free(clientFdPtr);
        close(clientFd);
        writeLog(logError, "Failed to create client thread", errno);
        return -1;
    }
    pthread_detach(threadId);
    writeLog(logInfo, "Client handler thread created successfully", 0);
    return 0;
}

int main() {

 // Инициализация логирования
    initLogger(LOG_FILE_PATH);

    char buffer[256];
    snprintf(buffer, sizeof(buffer), "Server started! PID: %d", getpid());
    writeLog(logInfo, buffer, 0);

    // Обработка завершения программы по ctrl+c через sigaction
    struct sigaction sa;
    sa.sa_handler = handleSignal;
    sigemptyset(&sa.sa_mask);
    sa.sa_flags = 0;
    sigaction(SIGINT, &sa, NULL);

    // Создаём главный сервер
    int serverFd = initServer(SERVER_PORT);
    if (serverFd == -1) {
        writeLog(logError, "server_fd", 0);
        exit(EXIT_FAILURE);
    }
    writeLog(logInfo, "Server started successfully", 0);

    while (keepRunning) {
        // socklen_t client_addr_size = sizeof(addr);
        // int clientFd = accept(serverFd, (struct sockaddr *)&addr, &client_addr_size);
        int clientFd = accept(serverFd, NULL, NULL);
        if (clientFd < 0) {
            if (errno == EINTR)
            {
                writeLog(logInfo, "Server interrupted by signal", 0);
                break; // Выходим из цикла, если нас прервал ctrl+c
            }
            else {
                writeLog(logError, "Accept failed", errno);
                continue;
            }
        }

        writeLog(logInfo, "New connection accepted", 0);

        if (runClientHandler(clientFd) < 0) {
            writeLog(logError, "Failed to create client handler", 0);
            continue;
        }
    }
    
    close(serverFd);
    writeLog(logInfo, "Server stopped", 0);
    closeLogger();
    exit(EXIT_SUCCESS);
}