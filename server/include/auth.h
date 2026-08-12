#ifndef AUTH_H
#define AUTH_H

#include "../../common/include/protocol.h"
#include "types.h"

#define USERS_CONFIG_PATH "srv/ftp/users.conf"

/**
 * @brief Функция авторизации клиента
 * @param login Логин пользователя
 * @param password Пароль пользователя
 * @param user Структура для заполнения данных сессии
 * @return ResponseCode Код ответа см common/include/protocol.h 
 */
ResponseCode authorizeClient(const char *login, const char *password, UserConfig *user);

/**
 * @brief Функция регистрации клиента
 * @param login Логин пользователя
 * @param password Пароль пользователя
 * @return ResponseCode Код ответа см common/include/protocol.h 
 */
ResponseCode registerClient(const char *login, const char *password);
// TODO ResponseCode findUser(const char *login, const char *password, UserConfig *users, int usersCount, UserConfig *result);

#endif // AUTH_H
