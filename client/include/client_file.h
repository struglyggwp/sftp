#ifndef CLIENT_FILE_H

#define CLIENT_FILE_H

#include "client_inet.h"

/**
 * @brief Скачивает файл с сервера, используя потоковую передачу.
 * @param fd Дескриптор сокета.
 * @param filename Имя файла для сохранения на диске.
 * @return int 0 при успехе, -1 при ошибке создания файла или обрыве связи.
 */
int recvFile(int fd, const char *filename);

/**
 * @brief Отправляет файл на сервер чанками по 4 КБ.
 * @param fd Дескриптор сокета.
 * @param filename Имя файла для отправки.
 * @return int 0 при успехе, -1 при ошибке открытия файла или отправки.
 */
int sendFile(int fd, const char *filename);

#endif // CLIENT_FILE_H