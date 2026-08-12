#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "client_inet.h"

int recvFile(int fd, const char *filename) {
    RequestHeader req;
    memset(&req, 0, sizeof(req));
    req.cmd = cmdDownload;
    req.dataLen = strlen(filename) + 1; // Длина имени файла + '\0'

    // Отправляем запрос (Заголовок + Имя файла)
    char *packet = buildPacket(&req, filename, req.dataLen);
    if (packet == NULL)
        return -1;

    if (sendMessage(fd, packet, sizeof(RequestHeader) + req.dataLen) != 0) {
        free(packet);
        return -1;
    }
    free(packet);

    // Читаем заголовок ответа
    ResponseHeader rsp;
    if (recvMessage(fd, &rsp, sizeof(rsp)) != (int)sizeof(rsp)) {
        return -1;
    }

    // Проверяем, нашел ли сервер файл
    if (rsp.code != rspSuccess) {
        responseCommand((char *)&rsp); // вывод ошибки
        return -1;
    }

    FILE *file = fopen(filename, "wb");
    if (file == NULL) {
        perror("Failed to create file");
        return -1;
    }

    // Вычитываем файл чанками (по 4 КБ)
    char buffer[4096];
    size_t remaining = rsp.dataLen; // Сколько байт осталось выкачать
    size_t totalReceived = 0;

    while (remaining > 0) {
        // Сколько байт просить у сокета 4096 или остаток файла, если он меньше 4096
        size_t toRead = (remaining < sizeof(buffer)) ? remaining : sizeof(buffer);

        ssize_t bytesRead = recvMessage(fd, buffer, toRead);
        if (bytesRead <= 0) {
            perror("Connection lost during download");
            fclose(file);
            return -1;
        }

        // Пишем на диск то, что пришло
        if (fwrite(buffer, 1, bytesRead, file) != (size_t)bytesRead) {
            perror("Failed to write file");
            fclose(file);
            return -1;
        }

        remaining -= bytesRead;
        totalReceived += bytesRead;
    }

    fclose(file);
    printf("Successfully downloaded '%s' (%zu bytes).\n", filename, totalReceived);
    return 0;
}

int sendFile(int fd, const char *filename) {
    // Открываем файл
    FILE *file = fopen(filename, "rb");
    if (file == NULL) {
        perror("Ошибка открытия файла для отправки");
        return -1;
    }

    // Узнаем размер файла для заголовка
    fseek(file, 0, SEEK_END);
    long fileSize = ftell(file);
    if (fileSize < 0) {
        fclose(file);
        return -1;
    }
    fseek(file, 0, SEEK_SET);

    // Формируем PushHeader
    PushHeader push;
    memset(&push, 0, sizeof(push));
    push.fileSize = fileSize;
    strncpy(push.fileName, filename, MAX_FILENAME - 1);
    push.fileName[MAX_FILENAME - 1] = '\0';

    // Формируем запрос (заголовок + PushHeader)
    RequestHeader req;
    memset(&req, 0, sizeof(req));
    req.cmd = cmdPush;
    req.dataLen = sizeof(PushHeader);

    // Отправляем заголовок и PushHeader как один пакет
    char *packet = buildPacket(&req, (const char *)&push, sizeof(PushHeader));
    if (packet == NULL) {
        fclose(file);
        return -1;
    }

    if (sendMessage(fd, packet, sizeof(RequestHeader) + sizeof(PushHeader)) != 0) {
        free(packet);
        fclose(file);
        return -1;
    }
    free(packet);
    // Отправляем содержимое файла чанками по 4 КБ
    char buffer[4096];
    size_t bytesRead;
    while ((bytesRead = fread(buffer, 1, sizeof(buffer), file)) > 0) {
        if (sendMessage(fd, buffer, bytesRead) != 0) {
            fprintf(stderr, "Ошибка при передаче данных файла\n");
            fclose(file);
            return -1;
        }
    }

    char *response = recvResponse(fd);
    if (response == NULL) {
        fclose(file);
        return -1;
    }

    fclose(file);

    responseCommand(response);
    free(response);

    return 0;
}
