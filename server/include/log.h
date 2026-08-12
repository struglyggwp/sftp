#ifndef LOG_H
#define LOG_H

#include "types.h"

/**
 * @brief Записывает сообщение в лог
 * Может передавать номер ошибки (errno) и потом превращать её в текст через strerror, или сразу передавать результат strerror
 * @param level Уровни логирования. см. struct LogLevel
 * @param message Сообщение для записи
 */
void writeLog(LogLevel level, const char *message, int errNo);

//TODO
int initLogger(const char *logPath);

//TODO
void closeLogger(void);

#endif // LOG_H
