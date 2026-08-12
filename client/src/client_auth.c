#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "client.h"
#include "client_inet.h"

static void clearInput(void) {
    int ch;
    while ((ch = getchar()) != '\n' && ch != EOF) {
    }
}

int clientAuth(int fd) {
    RequestHeader msg;
    AuthData data;
    size_t bufLen = sizeof(RequestHeader) + sizeof(AuthData);

    memset(&msg, 0, sizeof(RequestHeader));
    memset(&data, 0, sizeof(AuthData));

    int bufCmd;
    char bufCmdCh[3];
    char bufAuth[34];

    while (1) {

        while (1) {
            printf("Authentication/Registration? <1/2>\n");
            if (fgets(bufCmdCh, sizeof(bufCmdCh), stdin) == NULL) {
                return -1;
            }
            if (strchr(bufCmdCh, '\n') == NULL) {
                clearInput();
                printf("Invalid command\n");
                continue;
            }

            bufCmd = (int)bufCmdCh[0] - '0';
            if (bufCmd == 1)
                msg.cmd = cmdAuth;
            else if (bufCmd == 2)
                msg.cmd = cmdReg;
            else {
                printf("Invalid Command\n");
                continue;
            }
            break;
        }

        memset(&data, 0, sizeof(data));

        while (1) {
            printf("Login(No more than 32 characters): ");
            if (fgets(bufAuth, sizeof(bufAuth), stdin) == NULL) {
                perror("fgets error");
                return -1;
            }
            if (strchr(bufAuth, '\n') == NULL) {
                clearInput();
                printf("Invalid command\n");
                continue;
            }
            bufAuth[strcspn(bufAuth, "\n")] = '\0';
            strncpy(data.login, bufAuth, LEN_AUTH - 1);
            data.login[LEN_AUTH - 1] = '\0';
            break;
        }

        memset(bufAuth, 0, sizeof(bufAuth));

        while (1) {
            printf("Password(No more than 32 characters): ");
            if (fgets(bufAuth, sizeof(bufAuth), stdin) == NULL) {
                perror("fgets error");
                return -1;
            }
            if (strchr(bufAuth, '\n') == NULL) {
                clearInput();
                printf("Invalid command\n");
                continue;
            }
            bufAuth[strcspn(bufAuth, "\n")] = '\0';
            strncpy(data.pass, bufAuth, LEN_AUTH - 1);
            data.pass[LEN_AUTH - 1] = '\0';
            break;
        }

        msg.dataLen = sizeof(AuthData);

        char *buffer = buildPacket(&msg, &data, msg.dataLen);

        if (buffer == NULL)
            return -1;

        if (sendMessage(fd, buffer, bufLen) != 0) {
            free(buffer);
            return -1;
        }

        free(buffer);

        buffer = recvResponse(fd);

        if (buffer == NULL)
            return -1;

        if (responseCommand(buffer) == 0) {
            free(buffer);
            return 0;
        }

        free(buffer);
    }
}