#ifndef CLIENT_INET_H

#define CLIENT_INET_H

#include "../../common/include/protocol.h"

/**
 * @brief Устанавливает соединение с сервером.
 * @param argc Количество аргументов командной строки.
 * @param argv Аргументы (имя хоста, порт).
 * @return int Дескриптор сокета в случае успеха, или exit(1) при ошибке.
 */
int serverConnect(int argc, char *argv[]);

/**
 * @brief Закрывает соединение с сервером.
 * @param fd Дескриптор сокета.
 */
void serverDisconnect(int fd);

/**
 * @brief Безопасная отправка данных в сокет (гарантирует отправку всех байт).
 * @param fd Дескриптор сокета.
 * @param buffer Указатель на данные.
 * @param bufLen Размер данных.
 * @return int 0 при успехе, -1 при ошибке отправки.
 */
int sendMessage(int fd, const void *buffer, size_t bufLen);

/**
 * @brief Безопасное чтение данных из сокета.
 * @param fd Дескриптор сокета.
 * @param buffer Буфер для записи принятых данных.
 * @param bufLen Количество байт, которое нужно прочитать.
 * @return int Количество прочитанных байт или -1 при ошибке/обрыве связи.
 */
int recvMessage(int fd, void *buffer, size_t bufLen);

/**
 * @brief Парсит код ответа сервера и выводит соответствующее сообщение.
 * @param buffer Указатель на буфер с ответом (заголовок + данные).
 * @return int 0 если код успеха (rspSuccess, rspAuthSuccess и т.д.), -1 если ошибка.
 */
int responseCommand(char *buffer);

/**
 * @brief Отправляет запрос на сервер и ожидает ответ.
 * @param fd Дескриптор сокета.
 * @param cmd Команда протокола.
 * @param data Данные для отправки.
 * @param dataLen Размер данных в байтах.
 * @return char* Указатель на полученный ответ от сервера, либо NULL при ошибке.
 */
char *sendRequest(int fd, CommandType cmd, const void *data, size_t dataLen);

/**
 * @brief Принимает ответ от сервера, включая заголовок и тело сообщения.
 * @details Функция сначала считывает заголовок, а затем, опираясь на header.dataLen,
 * считывает оставшуюся часть данных.
 * @param fd Дескриптор сокета.
 * @return char* Указатель на буфер с ответом или NULL при ошибке.
 */
char *recvResponse(int fd);

/**
 * @brief Собирает пакет для отправки (Заголовок + Данные).
 * @param header Указатель на заголовок запроса.
 * @param data Указатель на данные.
 * @param dataSize Размер данных.
 * @return char* Выделенная память с готовым пакетом или NULL при ошибке malloc.
 */
char *buildPacket(const RequestHeader *header, const void *data, size_t dataSize);

#endif // CLIENT_INET_H
