#include "client_handler.h"
#include "auth.h"
#include "generators.h"
#include "network.h"
#include <pthread.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MESSAGE_BUFER 4096

extern volatile sig_atomic_t keepRunning;

int handleAuthRequest(int clientFd, RequestHeader *header, char *recvBuffer,
                      UserConfig *currentUser) {
    ResponseCode responseStatus = rspUnknown;
    AuthData *authData = (AuthData *)recvBuffer;
    char logMessage[128];

    if (header->cmd == cmdReg || header->cmd == cmdAuth) {
        if (header->dataLen != sizeof(AuthData)) {
            sendResponse(clientFd, rspBadRequest, NULL, 0);
            return 0;
        }
        authData->login[sizeof(authData->login) - 1] = '\0';
        authData->pass[sizeof(authData->pass) - 1] = '\0';
    }

    switch (header->cmd) {
    case cmdReg:
        responseStatus = registerClient(authData->login, authData->pass);
        if (responseStatus == rspRegSuccess) {
            responseStatus = authorizeClient(authData->login, authData->pass, currentUser);
        }
        break;
    case cmdAuth:
        responseStatus = authorizeClient(authData->login, authData->pass, currentUser);
        break;
    case cmdExit:
        return -1;
    default:
        sendResponse(clientFd, rspBadRequest, NULL, 0);
        return 0;
    }

    sendResponse(clientFd, responseStatus, NULL, 0);
    if (responseStatus == rspAuthSuccess) {
        snprintf(logMessage, sizeof(logMessage), "User %s authorized", currentUser->login);
        writeLog(logInfo, logMessage, 0);
        // Возвращаем 1, чтобы clientHandler вышел из цикла авторизации
        return 1;
    }

    if (header->cmd == cmdReg) {
        snprintf(logMessage, sizeof(logMessage), "Registration failed for user %s", authData->login);
    } else {
        snprintf(logMessage, sizeof(logMessage), "Authorization failed for user %s", authData->login);
    }
    writeLog(logInfo, logMessage, 0);

    return 0;
}

ResponseCode buildUserDirectory(const UserConfig *currentUser, char *userDir, size_t userDirSize) {
    char *maskPosition = NULL;

    if (currentUser == NULL || userDir == NULL || userDirSize == 0) {
        return rspBadRequest;
    }

    if (currentUser->userPath[0] == '\0' || strlen(currentUser->userPath) >= userDirSize) {
        return rspBadRequest;
    }

    strncpy(userDir, currentUser->userPath, userDirSize - 1);
    userDir[userDirSize - 1] = '\0';

    maskPosition = strstr(userDir, "*");
    if (maskPosition != NULL) {
        while (maskPosition > userDir && *maskPosition != '/') {
            maskPosition--;
        }
        *maskPosition = '\0';
    }

    return rspSuccess;
}

ResponseCode buildFilePath(const UserConfig *currentUser, char *fileName, char *filePath,
                           size_t filePathSize) {
    size_t needSize = 0;
    char userDir[MESSAGE_BUFER] = {0};

    if (currentUser == NULL || fileName == NULL || filePath == NULL) {
        return rspBadRequest;
    }

    if (fileName[0] == '\0') {
        return rspBadRequest;
    }

    if (strstr(fileName, "..") != NULL) {
        return rspAccessDenied;
    }

    if (strstr(fileName, "/") != NULL) {
        return rspAccessDenied;
    }

    if (buildUserDirectory(currentUser, userDir, sizeof(userDir)) != rspSuccess) {
        return rspBadRequest;
    }

    needSize = strlen(userDir) + 1 + strlen(fileName) + 1;
    if (needSize > filePathSize) {
        return rspBadRequest;
    }

    snprintf(filePath, filePathSize, "%s/%s", userDir, fileName);
    return rspSuccess;
}

ResponseCode buildUploadPath(const UserConfig *currentUser, char *filePath, size_t filePathSize) {
    char userDir[MESSAGE_BUFER] = {0};

    if (currentUser == NULL || filePath == NULL) {
        return rspBadRequest;
    }

    if (buildUserDirectory(currentUser, userDir, sizeof(userDir)) != rspSuccess) {
        return rspBadRequest;
    }

    if (strlen(userDir) + strlen("/uploaded_file") + 1 > filePathSize) {
        return rspBadRequest;
    }

    snprintf(filePath, filePathSize, "%s/uploaded_file", userDir);
    return rspSuccess;
}

int handleClientCommand(int clientFd, RequestHeader *header, char *recvBuffer, char *sendBuffer,
                        const UserConfig *currentUser) {
    ResponseCode responseStatus = rspUnknown;
    char filePath[MESSAGE_BUFER] = {0};
    char userDir[MESSAGE_BUFER] = {0};

    switch (header->cmd) {

    case cmdHelp: // Герерация справки
        memset(sendBuffer, 0, MESSAGE_BUFER);
        responseStatus = generateUserHelp(sendBuffer, MESSAGE_BUFER);
        if (responseStatus == rspSuccess) {
            sendResponse(clientFd, rspSuccess, sendBuffer, strlen(sendBuffer) + 1);
        } else {
            sendResponse(clientFd, responseStatus, NULL, 0);
        }
        break;
    case cmdList: // Список файлов
        memset(sendBuffer, 0, MESSAGE_BUFER);
        responseStatus = buildUserDirectory(currentUser, userDir, MESSAGE_BUFER);
        if (responseStatus != rspSuccess) {
            sendResponse(clientFd, responseStatus, NULL, 0);
            break;
        }

        responseStatus = generateFileList(userDir, sendBuffer, MESSAGE_BUFER);
        if (responseStatus == rspSuccess) {
            sendResponse(clientFd, rspSuccess, sendBuffer, strlen(sendBuffer) + 1);
        } else {
            sendResponse(clientFd, responseStatus, NULL, 0);
        }
        break;
    case cmdDownload:
        // Клиент должен прислать только имя файла
        if (header->dataLen == 0 || header->dataLen >= MESSAGE_BUFER) {
            sendResponse(clientFd, rspBadRequest, NULL, 0);
            break;
        }

        recvBuffer[header->dataLen] = '\0';

        responseStatus = buildFilePath(currentUser, recvBuffer, filePath, MESSAGE_BUFER);
        if (responseStatus != rspSuccess) {
            sendResponse(clientFd, responseStatus, NULL, 0);
            break;
        }

        if (sendFile(clientFd, filePath) >= 0) {
            writeLog(logInfo, "File downloaded", 0);
        }
        break;
    case cmdPush:
    {
        writeLog(logInfo, "PUSH command", 0);
        // Клиент отправляет файл на сервер. Маску тут не проверяем
        PushHeader *push = (PushHeader *)recvBuffer;

        // Проверяем, что получили полный PushHeader
        if (header->dataLen != sizeof(PushHeader)) {
            sendResponse(clientFd, rspBadRequest, NULL, 0);
            break;
        }

        // Проверяем, что мы уже получили PushHeader
        responseStatus = buildFilePath(currentUser, push->fileName, filePath, sizeof(filePath));

        if (responseStatus != rspSuccess) {
            sendResponse(clientFd, responseStatus, NULL, 0);
            break;
        }

        // Принимаем файл
        if (recvFile(clientFd, filePath, push->fileSize) < 0) {
            sendResponse(clientFd, rspServerError, NULL, 0);
            break;
        }

        writeLog(logInfo, "File uploaded", 0);

        sendResponse(clientFd, rspSuccess, NULL, 0);
        break;
    }
    case cmdExit:
        return -1;
    default:
        sendResponse(clientFd, rspBadRequest, NULL, 0);
        break;
    }

    return 0;
}

void *clientHandler(void *arg) {

    int clientFd = *(int *)arg;
    free(arg);
    // Делаем сокет неблокирующим
    makeSocketTimeout(clientFd, 1);

    char recvBuffer[MESSAGE_BUFER] = {0};
    char sendBuffer[MESSAGE_BUFER] = {0};
    UserConfig currentUser = {0};

    // Цикл авторизации
    while (keepRunning) {
        RequestHeader header = {0};
        int bytes = recvRequest(clientFd, &header, recvBuffer, sizeof(recvBuffer));
        if (bytes == -2)
            continue; //Таймаут, читаем по новой
        if (bytes <= 0) {
            writeLog(logInfo, "Client disconnected during authorization", 0);
            close(clientFd);
            pthread_exit(NULL);
        }

        int authResult = handleAuthRequest(clientFd, &header, recvBuffer, &currentUser);
        if (authResult < 0) {
            close(clientFd);
            pthread_exit(NULL);
        }
        if (authResult > 0) {
            break;
        }
    }

    // Главный цикл
    while (keepRunning) {
        RequestHeader header = {0};
        int bytes = recvRequest(clientFd, &header, recvBuffer, sizeof(recvBuffer));
        if (bytes == -2)
            continue; // Таймаут, читаем по новой
        if (bytes <= 0)
            break;

        if (handleClientCommand(clientFd, &header, recvBuffer, sendBuffer, &currentUser) < 0) {
            close(clientFd);
            pthread_exit(NULL);
        }
    }

    close(clientFd);
    writeLog(logInfo, "Client disconnected", 0);
    pthread_exit(NULL);
}