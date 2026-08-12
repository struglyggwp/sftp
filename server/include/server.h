#ifndef SERVER_H
#define SERVER_H

void handleSignal(int sig);

/**
 * @brief Инициализирует сервер
 * Создаёт сокет, делает привязку и начинает слушать входящие сообщения с любого ip на указанный порт
 * @param port
 * @return int -1 - Ошибка, 0 - Успех
 */
int initServer(int port);

/**
 * @brief Создаёт поток для взаимодействия с клиентом
 * Создаёт поток, и созданному потоку отдаёт функцию для работы с клиентом.
 * Так же в эту функцию передаем дескриптор клиента
 * @param clientFd
 * @return -1 - Ошибка, 0 - Успех
 */
int runClientHandler(int clientFd);

#endif // SERVER_H
