#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

#include "../../common/include/protocol.h"
#include "client_inet.h"

int serverConnect(int argc, char *argv[]) {

    int fdServer;

    if (argc != 3) {
        printf("Use  ./client <ip-server> <port>\n");
        return 1;
    }

    if ((fdServer = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("socket error");
        exit(EXIT_FAILURE);
    }

    char *ipServer = argv[1];
    int portServer = atoi(argv[2]);
    struct sockaddr_in serverSockAddr;

    memset(&serverSockAddr, 0, sizeof(serverSockAddr));
    serverSockAddr.sin_family = AF_INET;
    serverSockAddr.sin_port = htons(portServer);

    if (inet_pton(AF_INET, ipServer, &serverSockAddr.sin_addr) != 1) {
        perror("ip address invalid error");
        exit(EXIT_FAILURE);
    }

    if (connect(fdServer, (struct sockaddr *)&serverSockAddr, sizeof(serverSockAddr)) == -1) {
        perror("connect to server error");
        exit(EXIT_FAILURE);
    }

    return fdServer;
}

void serverDisconnect(int fd) {
    if (close(fd) == -1) {
        perror("fd close error");
        exit(EXIT_FAILURE);
    }
}

char *buildPacket(const RequestHeader *header, const void *data, size_t dataSize) {
    size_t packetSize = sizeof(RequestHeader) + dataSize;

    char *packet = malloc(packetSize);
    if (packet == NULL) {
        perror("malloc");
        return NULL;
    }

    memcpy(packet, header, sizeof(RequestHeader));

    if (data != NULL && dataSize > 0)
        memcpy(packet + sizeof(RequestHeader), data, dataSize);

    return packet;
}

char *recvResponse(int fd) {
    ResponseHeader header;

    if (recvMessage(fd, &header, sizeof(header)) != (int)sizeof(header))
        return NULL;

    size_t packetSize = sizeof(header) + header.dataLen;

    char *packet = malloc(packetSize);

    if (packet == NULL)
        return NULL;

    memcpy(packet, &header, sizeof(header));

    if (header.dataLen > 0) {
        if (recvMessage(fd, packet + sizeof(header), header.dataLen) != (int)header.dataLen) {
            free(packet);
            return NULL;
        }
    }

    return packet;
}

char *sendRequest(int fd, CommandType cmd, const void *data, size_t dataLen) {
    RequestHeader header;

    memset(&header, 0, sizeof(header));

    header.cmd = cmd;
    header.dataLen = dataLen;

    char *packet = buildPacket(&header, data, dataLen);

    if (packet == NULL)
        return NULL;

    int result = sendMessage(fd, packet, sizeof(RequestHeader) + dataLen);

    free(packet);

    if (result != 0)
        return NULL;

    if (cmd == cmdExit) {
        return NULL; // Не ждем ответа для EXIT
    }

    char *response = recvResponse(fd);

    if (response == NULL)
        return NULL;

    responseCommand(response);

    return response;
}

int sendMessage(int fd, const void *buffer, size_t bufLen) {
    size_t total = 0;

    while (total < bufLen) {
        ssize_t bytesSent = send(fd, (const char *)buffer + total, bufLen - total, 0);

        if (bytesSent <= 0) {
            perror("send");
            return -1;
        }

        total += bytesSent;
    }

    return 0;
}

int recvMessage(int fd, void *buffer, size_t bufLen) {
    int total = 0;

    while (total < (int)bufLen) {
        int bytesRead = recv(fd, (char *)buffer + total, bufLen - total, 0);

        if (bytesRead <= 0) {
            perror("recv");
            return -1;
        }

        total += bytesRead;
    }

    return total;
}
int responseCommand(char *buffer) {

    ResponseHeader *response = (ResponseHeader *)buffer;

    switch (response->code) {
    case rspAuthSuccess:
        printf("Authentication success.\n");
        return 0;
    case rspRegSuccess:
        printf("Registratrion success.\n");
        return 0;
    case rspAuthError:
        printf("Authentication failure.\n");
        return -1;
    case rspRegError:
        printf("Registratrion failure.\n");
        return -1;
    case rspSuccess:
        printf("Request success.\n");
        return 0;
    case rspBadRequest:
        printf("Bad request.\n");
        return -1;
    case rspFileNotFound:
        printf("File not found.\n");
        return -1;
    case rspAccessDenied:
        printf("Access denied.\n");
        return -1;
    case rspServerError:
        printf("Server error.\n");
        return -1;
    case rspUnknown:
        printf("Unknown response.\n");
        return -1;
    }
    return -1;
}
