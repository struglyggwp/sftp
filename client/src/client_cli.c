#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "client_cli.h"
#include "client_file.h"
#include "client_inet.h"

static CommandMap commands[] = {{"HELP", cmdHelp},
                                {"LIST", cmdList},
                                {"GET", cmdDownload},
                                {"EXIT", cmdExit},
                                {"PUSH", cmdPush}};

int cliRun(int fd, char *buffer, size_t bufSize) {
    printf("sftp>");
    fflush(stdout);
    char *response;

    if (fgets(buffer, bufSize, stdin) == NULL)
        return 0;

    buffer[strcspn(buffer, "\n")] = '\0';

    if (strlen(buffer) == 0)
        return 1;

    RequestHeader header;
    char *data;

    if (parseCli(buffer, &header, &data) == 0) {

        switch (header.cmd) {
        case cmdExit:
            response = sendRequest(fd, header.cmd, data, header.dataLen);
            free(response);
            return 0;
        case cmdDownload:
            if (data == NULL || strlen(data) == 0) {
                printf("Error (use GET <file.txt>)\n");
            } else {
                recvFile(fd, data);
            }
            break;
        case cmdPush:
            if (data == NULL || strlen(data) == 0) {
                printf("Error (use PUSH <file.txt>)\n");
            } else {
                sendFile(fd, data);
            }
            break;
        default:
            response = sendRequest(fd, header.cmd, data, header.dataLen);
            if (response != NULL) {
                ResponseHeader *rspHeader = (ResponseHeader *)response;

                if (rspHeader->dataLen > 0) {
                    char *responseData = response + sizeof(ResponseHeader);

                    printf("%.*s", (int)rspHeader->dataLen, responseData);
                }
                free(response);
            } else {
                printf("Error: Failed to receive response from server.\n");
            }
            break;
        }
    }
    return 1;
}

int parseCli(char *input, RequestHeader *header, char **data) {
    char *cmd = strtok(input, " ");

    if (cmd == NULL)
        return -1;

    header->cmd = cmdUnknown;
    header->dataLen = 0;

    *data = NULL;

    for (size_t i = 0; i < sizeof(commands) / sizeof(commands[0]); i++) {
        if (strcasecmp(cmd, commands[i].name) == 0) {
            header->cmd = commands[i].cmd;
            break;
        }
    }

    if (header->cmd == cmdUnknown)
        return -1;

    // Аргументы после команды
    char *arg = strtok(NULL, " \t\n\r"); // Разделитель пробел или таб

    if (arg != NULL) {
        *data = arg;
        header->dataLen = strlen(arg) + 1;
    }

    return 0;
}
