#ifndef NETWORK_H
#define NETWORK_H

#include <stddef.h>
#include "../../common/include/protocol.h"

// TODO
int sendAll(int fd, void *buffer, size_t bytesToSend);

// TODO
int recvFull(int fd, void *buffer, size_t bytesToRead);

// TODO
int recvRequest(int fd, RequestHeader *header, char *buffer, size_t bufferSize);

// TODO
int sendResponse(int fd, ResponseCode code, void *data, size_t dataLen);

// TODO
int sendFile(int clientFd, const char *filePath);

// TODO
int recvFile(int clientFd, const char *filePath, size_t fileSize);

/**
 * @brief Делает сокет неблокирующим
 * @param clientFd
 * @param seconds Секунд до разблокировки
 */
void makeSocketTimeout(int clientFd, int seconds);

#endif // NETWORK_H
