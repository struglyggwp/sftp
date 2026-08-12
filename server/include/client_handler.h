#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

#include "../../common/include/protocol.h"
#include "types.h"

/**
 * @brief Основная функция для работы с клиентом
 * Тут мы получаем и отправляем сообщения клиенту!
 * @param arg Принимаем дескриптор клиента, так что нужно привести его к int
 * @return void*
 */
void *clientHandler(void *arg);

// TODO
int handleAuthRequest(int clientFd, RequestHeader *header, char *recvBuffer, UserConfig *currentUser);

// TODO
ResponseCode buildUserDirectory(const UserConfig *currentUser, char *userDir, size_t userDirSize);

// TODO
ResponseCode buildFilePath(const UserConfig *currentUser, char *fileName, char *filePath, size_t filePathSize);

// TODO
ResponseCode buildUploadPath(const UserConfig *currentUser, char *filePath, size_t filePathSize);

// TODO
int handleClientCommand(int clientFd, RequestHeader *header, char *recvBuffer, char *sendBuffer, const UserConfig *currentUser);

#endif // CLIENT_HANDLER_H
