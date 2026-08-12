#define _POSIX_C_SOURCE 200809L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <errno.h>
#include <unistd.h>
#include "generators.h"
#include "log.h"
#include "types.h"

// Генерация справки
ResponseCode generateUserHelp(char *buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0) {
        writeLog(logError, "generateUserHelp: invalid buffer", 0);
        return rspServerError;
    }

    memset(buffer, 0, bufferSize);

    const char *helpText =
        "=== FTP Server Commands ===\n"
        "Available commands:\n"
        "HELP              - Show this help\n"
        "LIST              - List available files\n"
        "GET <file>        - Download file\n"
        "PUSH <file>       - Upload file\n"
        "EXIT              - Exit connection\n";

    size_t len = strlen(helpText);
    if (len >= bufferSize) {
        strncpy(buffer, helpText, bufferSize - 1);
        buffer[bufferSize - 1] = '\0';
        writeLog(logInfo, "generateUserHelp: buffer too small, truncated", 0);
        return rspSuccess;
    }
    
    strcpy(buffer, helpText);
    return rspSuccess;
}

// Проверка, является ли файл обычным файлом
static int isRegularFile(const char *path) {
    struct stat statbuf;
    if (stat(path, &statbuf) != 0) {
        return 0;
    }
    return S_ISREG(statbuf.st_mode);
}

// Генерация списка файлов
ResponseCode generateFileList(const char *userPath, char *buffer, size_t bufferSize) {
    if (buffer == NULL || bufferSize == 0 || userPath == NULL) {
        writeLog(logError, "generateFileList: invalid parameters", 0);
        return rspBadRequest;
    }

    memset(buffer, 0, bufferSize);

    DIR *dir = opendir(userPath);
    if (dir == NULL) {
        writeLog(logError, "Failed to open user directory", errno);
        snprintf(buffer, bufferSize, "Error: Cannot open directory '%s'\n", userPath);
        return rspFileNotFound;
    }

    struct dirent *entry;
    size_t currentPos = 0;
    int filesFound = 0;

    currentPos += snprintf(buffer + currentPos, bufferSize - currentPos, 
                          "=== Files in %s ===\n", userPath);

    while ((entry = readdir(dir)) != NULL) {
        if (entry->d_name[0] == '.') {
            continue;
        }

        char fullPath[512];
        snprintf(fullPath, sizeof(fullPath), "%s/%s", userPath, entry->d_name);
        
        if (!isRegularFile(fullPath)) {
            continue;
        }

        if (currentPos + strlen(entry->d_name) + 40 >= bufferSize) {
            snprintf(buffer + currentPos, bufferSize - currentPos, "...\n");
            break;
        }

        struct stat fileStat;
        if (stat(fullPath, &fileStat) == 0) {
            char sizeStr[32];
            if (fileStat.st_size < 1024) {
                snprintf(sizeStr, sizeof(sizeStr), "%ld B", fileStat.st_size);
            } else if (fileStat.st_size < 1024 * 1024) {
                snprintf(sizeStr, sizeof(sizeStr), "%.1f KB", (double)fileStat.st_size / 1024);
            } else {
                snprintf(sizeStr, sizeof(sizeStr), "%.1f MB", (double)fileStat.st_size / (1024 * 1024));
            }
            currentPos += snprintf(buffer + currentPos, bufferSize - currentPos, 
                                  "  %-30s [%s]\n", entry->d_name, sizeStr);
        } else {
            currentPos += snprintf(buffer + currentPos, bufferSize - currentPos, 
                                  "  %s\n", entry->d_name);
        }
        filesFound++;
    }

    closedir(dir);

    if (filesFound == 0 && currentPos < bufferSize) {
        snprintf(buffer + currentPos, bufferSize - currentPos, 
                "No files found in directory\n");
    }

    if (currentPos + 30 < bufferSize) {
        snprintf(buffer + currentPos, bufferSize - currentPos, 
                "Total: %d file(s)\n", filesFound);
    }

    return rspSuccess;
}