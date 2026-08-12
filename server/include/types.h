#ifndef TYPES_H
#define TYPES_H

#include "../../common/include/protocol.h"

// Уровни логирования для записи в лог
// TODO наверно не нужна, мы в writeLog можем сразу errno передавать
typedef enum {
    logInfo,
    logError
} LogLevel;

/* Можно сделать глобальный массив структур типа UserConfig, и хранить там данные пользователей.
 * А можно просто файлик каждый раз перечитывать
 * Тут надо решить как мы будем это делать */
typedef struct {
    char login[32];
    char password[32];
    char userPath[256];
    //char mask[32];
} UserConfig;

#endif // TYPES_H
