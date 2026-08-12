#ifndef COMMON_PROTOCOL_H
#define COMMON_PROTOCOL_H

#include <stddef.h>

#define MAX_FILENAME 256

typedef enum {
    cmdHelp,
    cmdList,
    cmdDownload,
    cmdPush,
    cmdReg,
    cmdAuth,
    cmdExit,
    cmdUnknown
} CommandType;

// TODO
typedef enum {
    rspUnknown = -1,
    rspSuccess,
    rspRegSuccess,
    rspRegError,
    rspAuthSuccess,
    rspAuthError,
    rspBadRequest,
    rspFileNotFound,
    rspAccessDenied,
    rspServerError
} ResponseCode;

// TODO
typedef struct {
    CommandType cmd;
    size_t dataLen;
} RequestHeader;

// TODO
typedef struct {
    ResponseCode code;
    size_t dataLen;
} ResponseHeader;


typedef struct {
    size_t fileSize;
    char fileName[MAX_FILENAME];
} PushHeader;

// TODO
typedef struct {
    char login[32];
    char pass[32];
} AuthData;

#endif // COMMON_PROTOCOL_H
