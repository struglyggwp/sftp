#include "network.h"
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include "log.h"
#include "network.h"

extern volatile sig_atomic_t keepRunning;

int recvRequest(int fd, RequestHeader *header, char *buffer, size_t bufferSize) {
    // Сначала читаем заголовок, чтобы узнать команду и размер данных
    int bytes = recvFull(fd, header, sizeof(RequestHeader));
    // Если заголовок не прочитался - возвращаем ошибку
    if (bytes <= 0) {
        return bytes;
    }

    // Если команда вне нашего enum - считаем её неизвестной
    if (header->cmd < cmdHelp || header->cmd > cmdUnknown) {
        header->cmd = cmdUnknown;
    }

    // При загрузке файла тело сообщения читается отдельно в recvFile()
    if (header->cmd == cmdPush) {
        if (header->dataLen != sizeof(PushHeader)) {
            return -1;
        }
        return recvFull(fd, buffer, sizeof(PushHeader));
    }
    // Что делать с данными если они больше буфера? В данном случае мы вернём ошибку
    if (header->dataLen > bufferSize) {
        writeLog(logError, "Request data is too large", 0);
        return -1;
    }

    // Данных нет
    if (header->dataLen == 0) {
        return bytes;
    }

    // Если dataLen больше нуля - читаем данные из сообщения
    bytes = recvFull(fd, buffer, header->dataLen);
    if (bytes <= 0) {
        return bytes;
    }

    return bytes;
}

int recvFull(int fd, void *buffer, size_t bytesToRead) {
    char *current = buffer;
    size_t totalBytes = 0;

    while (keepRunning && totalBytes < bytesToRead) {
        // recv может прочитать меньше, чем мы попросили
        int bytes = recv(fd, current + totalBytes, bytesToRead - totalBytes, 0);
        if (bytes == 0) {
            // 0 значит клиент закрыл соединение
            return 0;
        }
        if (bytes < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                if (totalBytes == 0) {
                    // Данных пока нет. Таймаут сокета
                    return -2;
                }
                // Часть данных уже есть пробуем дочитать остальные
                continue;
            }
            // Любая другая ошибка recv - будем считать просто ошибкой
            writeLog(logError, "Socket receive error", errno);
            return -1;
        }

        totalBytes += bytes;
    }

    if (totalBytes == bytesToRead) {
        return (int)totalBytes;
    }

    return -1;
}

int sendResponse(int fd, ResponseCode code, void *data, size_t dataLen) {
    ResponseHeader header;
    header.code = code;
    header.dataLen = dataLen;

    if (dataLen > 0 && data == NULL) {
        writeLog(logError, "Response data is NULL", 0);
        return -1;
    }

    // Общий пакет (заголовок + данные)
    size_t packetSize = sizeof(ResponseHeader) + dataLen;
    char *packet = malloc(packetSize);
    if (packet == NULL) {
        writeLog(logError, "Response memory allocation error", errno);
        return -1;
    }

    // Заголовок в начало пакета
    memcpy(packet, &header, sizeof(ResponseHeader));

    // Данные кладём после заголовка (привет домашка с RAW сокетами :D )
    if (dataLen > 0 && data != NULL) {
        memcpy(packet + sizeof(ResponseHeader), data, dataLen);
    }

    // Отправляем общим буфером
    int bytes = sendAll(fd, packet, packetSize);
    free(packet);
    return bytes;
}

int sendAll(int fd, void *buffer, size_t bytesToSend) {
    char *current = buffer;
    size_t totalBytes = 0;

    while (totalBytes < bytesToSend) {
        // send может отправить только часть буфера
        int bytes = send(fd, current + totalBytes, bytesToSend - totalBytes, 0);
        if (bytes <= 0) {
            writeLog(logError, "Socket send error", errno);
            return -1;
        }

        totalBytes += bytes;
    }

    // Возвращаем сколько байт реально отправили
    return (int)totalBytes;
}

int sendFile(int clientFd, const char *filePath) {
    char buffer[4096];
    struct stat fileInfo;

    // Открываем файл
    int fileFd = open(filePath, O_RDONLY);
    if (fileFd < 0) {
        writeLog(logError, "Failed to open file for download", errno);
        if (errno == ENOENT) {
            sendResponse(clientFd, rspFileNotFound, NULL, 0);
        } else if (errno == EACCES) {
            sendResponse(clientFd, rspAccessDenied, NULL, 0);
        } else {
            sendResponse(clientFd, rspServerError, NULL, 0);
        }
        return -1;
    }

    // Узнаём размер файла
    if (fstat(fileFd, &fileInfo) < 0) {
        writeLog(logError, "Failed to get file size", errno);
        close(fileFd);
        sendResponse(clientFd, rspServerError, NULL, 0);
        return -1;
    }

    ResponseHeader header;
    header.code = rspSuccess;
    header.dataLen = (size_t)fileInfo.st_size;

    // Сначала отправим заголовок
    if (sendAll(clientFd, &header, sizeof(header)) < 0) {
        writeLog(logError, "Failed to send file header", 0);
        close(fileFd);
        return -1;
    }

    // Файл отправим по кускам
    size_t totalBytes = 0;
    while (totalBytes < header.dataLen) {
        // Читаем кусок
        int bytes = read(fileFd, buffer, sizeof(buffer));
        if (bytes <= 0) {
            writeLog(logError, "Failed to read download file", errno);
            close(fileFd);
            return -1;
        }

        // И отправляем его
        if (sendAll(clientFd, buffer, bytes) < 0) {
            writeLog(logError, "Failed to send file data", 0);
            close(fileFd);
            return -1;
        }

        totalBytes += bytes;
    }

    close(fileFd);
    return (int)totalBytes;
}

int recvFile(int clientFd, const char *filePath, size_t fileSize) {
    char buffer[4096];
    size_t totalBytes = 0;

    int fileFd = open(filePath, O_CREAT | O_WRONLY | O_TRUNC, 0644);
    if (fileFd < 0) {
        writeLog(logError, "Failed to open upload file", errno);
        return -1;
    }

    while (totalBytes < fileSize) {
        size_t bytesToRead = sizeof(buffer);
        if (fileSize - totalBytes < bytesToRead) {
            bytesToRead = fileSize - totalBytes;
        }

        int bytes = recvFull(clientFd, buffer, bytesToRead);
        if (bytes == -2) {
            continue;
        }

        if (bytes <= 0)
        {
            writeLog(logError, "Failed to receive file data", 0);
            // Реальная ошибка или разрыв соединения
            close(fileFd);
            unlink(filePath); // Удаляем неполный файл
            return -1;
        }

        if (write(fileFd, buffer, bytes) != bytes) {
            writeLog(logError, "Failed to write upload file", errno);
            close(fileFd);
            return -1;
        }

        totalBytes += bytes;
    }

    close(fileFd);
    return (int)totalBytes;
}

void makeSocketTimeout(int clientFd, int seconds) {
    struct timeval timeout;
    timeout.tv_sec = seconds;
    timeout.tv_usec = 0;

    if (setsockopt(clientFd, SOL_SOCKET, SO_RCVTIMEO, (const char *)&timeout, sizeof(timeout)) < 0) {
        writeLog(logError, "Failed to set socket timeout", errno);
    }
}
