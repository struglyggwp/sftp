#ifndef CLIENT_CLI_H
#define CLIENT_CLI_H

#include "../../common/include/protocol.h"
#include <stddef.h>

typedef struct {
    const char *name;
    CommandType cmd;
} CommandMap;

/**
 * @brief Основной цикл обработки команд пользователя.
 * * Читает строку из stdin, вызывает парсер и выполняет соответствующую команду.
 * * @param fd Дескриптор сокета для отправки запросов на сервер.
 * @param buffer Указатель на буфер для чтения ввода.
 * @param bufSize Размер буфера.
 * @return int Возвращает 1 для продолжения работы, 0 если нужно завершить программу (EXIT).
 */
int cliRun(int fd, char *buffer, size_t bufSize);

/**
 * @brief Парсит введенную пользователем строку и заполняет структуру запроса.
 * * Преобразует строковую команду в enum-код и извлекает аргумент (имя файла).
 * * @param input Исходная строка ввода.
 * @param header Указатель на структуру заголовка, которую нужно заполнить.
 * @param data Указатель на указатель (будет указывать на аргумент в строке ввода).
 * @return int 0 если команда успешно распознана, -1 в случае ошибки.
 */
int parseCli(char *input, RequestHeader *header, char **data);

#endif