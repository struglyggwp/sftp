#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <pthread.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "log.h"
#include "types.h"

static char logFilePath[512] = "/srv/log.txt";
static FILE *logFile = NULL;
static pthread_mutex_t logMutex = PTHREAD_MUTEX_INITIALIZER;
static int isInitialized = 0;

// Преобразование уровня логирования в строку
static const char* logLevelToString(LogLevel level) {
    switch (level) {
        case logInfo: return "INFO";
        case logError: return "ERROR";
        default: return "UNKNOWN";
    }
}

// Получение текущего времени в строковом формате
static void getCurrentTime(char *buffer, size_t size) {
    time_t now = time(NULL);
    struct tm *timeInfo = localtime(&now);
    strftime(buffer, size, "%Y-%m-%d %H:%M:%S", timeInfo);
}

// Создание директории для логов если её нет
static void createLogDirectory(const char *path) {
    char dirPath[512];
    strncpy(dirPath, path, sizeof(dirPath) - 1);
    dirPath[sizeof(dirPath) - 1] = '\0';
    
    char *lastSlash = strrchr(dirPath, '/');
    if (lastSlash != NULL) {
        *lastSlash = '\0';
        mkdir(dirPath, 0755);
    }
}

int initLogger(const char *logPath) {
    pthread_mutex_lock(&logMutex);
    
    if (logPath != NULL) {
        strncpy(logFilePath, logPath, sizeof(logFilePath) - 1);
        logFilePath[sizeof(logFilePath) - 1] = '\0';
    }
    
    // Создаем директорию
    createLogDirectory(logFilePath);
    
    logFile = fopen(logFilePath, "a");
    if (logFile == NULL) {
        pthread_mutex_unlock(&logMutex);
        return -1;
    }
    
    setvbuf(logFile, NULL, _IOLBF, 0);
    isInitialized = 1;
    
    // Пишем первую запись
    char timeStr[64];
    getCurrentTime(timeStr, sizeof(timeStr));
    fprintf(logFile, "[%s] [INFO] === Server logging initialized ===\n", timeStr);
    fflush(logFile);
    
    pthread_mutex_unlock(&logMutex);
    return 0;
}

void writeLog(LogLevel level, const char *message, int errNo) {
    if (!isInitialized) {
        if (initLogger(NULL) != 0) {
            fprintf(stderr, "Log init failed: %s\n", message ? message : "NULL message");
            return;
        }
    }
    
    if (message == NULL) {
        return;
    }
    
    pthread_mutex_lock(&logMutex);
    
    char timeStr[64];
    char logMessage[1024];
    getCurrentTime(timeStr, sizeof(timeStr));
    
    if (errNo != 0) {
        snprintf(logMessage, sizeof(logMessage), "[%s] [%s] %s (errno: %d - %s)\n",
                 timeStr, logLevelToString(level), message, errNo, strerror(errNo));
    } else {
        snprintf(logMessage, sizeof(logMessage), "[%s] [%s] %s\n", timeStr, logLevelToString(level), message);
    }

    if (level == logError) {
        fprintf(stderr, "%s", logMessage);
    } else {
        fprintf(stdout, "%s", logMessage);
    }

    fprintf(logFile, "%s", logMessage);
    fflush(logFile);
    pthread_mutex_unlock(&logMutex);
}


void closeLogger(void) {
    if (logFile != NULL) {
        char timeStr[64];
        getCurrentTime(timeStr, sizeof(timeStr));
        fprintf(logFile, "[%s] [INFO] === Server logging stopped ===\n", timeStr);
        fflush(logFile);
        fclose(logFile);
        logFile = NULL;
        isInitialized = 0;
    }
}