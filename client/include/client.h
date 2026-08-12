#ifndef CLIENT_H
#define CLIENT_H

#include "../../common/include/protocol.h"

#define LEN_PACKET 4096
#define LEN_AUTH 32

// АВТОРИЗАЦИЯ\РЕГИСТРАЦИЯ
int clientAuth(int fd);



#endif