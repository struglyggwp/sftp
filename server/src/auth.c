#include "../include/auth.h"
#include "../include/log.h"
#include <stdio.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>

ResponseCode authorizeClient(const char *login, const char *password, UserConfig *user) {
    if (login == NULL || password == NULL || user == NULL) {
        return rspAuthError;
    }

    if (strlen(login) >= sizeof(user->login) || strlen(password) >= sizeof(user->password)) {
        return rspAuthError;
    }

    FILE *file = fopen(USERS_CONFIG_PATH, "r");
    if (file == NULL) {
        writeLog(logError, "fopen error", errno);
        return rspAuthError;
    }

    char buffer[512];
    ResponseCode code = rspAuthError;

    while (fgets(buffer, sizeof(buffer), file) != NULL) {
        buffer[strcspn(buffer, "\n")] = '\0';

        if (strlen(buffer) == 0) {
            continue; 
        }

        char *saveptr;
        char *file_login = strtok_r(buffer, ":", &saveptr);
        char *file_password = strtok_r(NULL, ":", &saveptr);
        char *file_mask = strtok_r(NULL, ":", &saveptr);

        if (file_login == NULL || file_password == NULL || file_mask == NULL) {
            continue;
        }

        if (strcmp(login, file_login) == 0 && strcmp(password, file_password) == 0) {
            if (strlen(file_mask) >= sizeof(user->userPath)) {
                fclose(file);
                return rspAuthError;
            }
            strcpy(user->login, file_login);
            strcpy(user->password, file_password);
            strcpy(user->userPath, file_mask);
            code = rspAuthSuccess;
            break;
        }
    }

    fclose(file);
    return code;
}

ResponseCode registerClient(const char *login, const char *password) {
    if (login == NULL || password == NULL) {
        return rspRegError;
    }
    
    if (login[0] == '\0' || password[0] == '\0' || strlen(login) >= 32 || strlen(password) >= 32) {
        return rspRegError;
    }

    if (strstr(login, "..") != NULL || strpbrk(login, "/:\r\n") != NULL ||
        strpbrk(password, ":\r\n") != NULL) {
        return rspRegError;
    }

    mkdir("srv", 0755);
    mkdir("srv/ftp", 0755);

    FILE *file = fopen(USERS_CONFIG_PATH, "r");
    if (file != NULL) {
        char buffer[512];
        while (fgets(buffer, sizeof(buffer), file) != NULL) {
            buffer[strcspn(buffer, "\n")] = '\0';

            if (strlen(buffer) == 0) {
                continue; 
            }
            char *saveptr;
            char *file_login = strtok_r(buffer, ":", &saveptr);
            if (file_login != NULL && strcmp(login, file_login) == 0) {
                writeLog(logInfo, "Registration failed: user already exists", 0);
                fclose(file);
                return rspRegError;
            }
        }

        fclose(file);
    }

    file = fopen(USERS_CONFIG_PATH, "a");
    if (file == NULL) {
        writeLog(logError, "fopen error", errno);
        return rspFileNotFound;
    }

    char dir_name[256];
    snprintf(dir_name, sizeof(dir_name), "srv/ftp/%s", login);
    if (mkdir(dir_name, S_IRWXU | S_IRGRP | S_IXGRP | S_IROTH | S_IXOTH) == -1) {
        if (errno == EEXIST) {
            writeLog(logInfo, "mkdir warning (user dir)", errno); 
        } else {
            writeLog(logError, "mkdir error (user dir)", errno);
            fclose(file);
            return rspRegError;
        }
    }

    fprintf(file, "%s:%s:srv/ftp/%s/*\n", login, password, login);

    fclose(file);
    writeLog(logInfo, "User registered", 0);
    return rspRegSuccess;
}