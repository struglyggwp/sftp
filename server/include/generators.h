#ifndef GENERATORS_H
#define GENERATORS_H

#include <stddef.h>
#include "../../common/include/protocol.h"

/**
  * @brief Генерирует справку в предоставленный буфер
  * @param buffer 
  * @param bufferSize 
  * @return ResponseCode Код ответа см common/include/protocol.h 
  */
ResponseCode generateUserHelp(char *buffer, size_t bufferSize);

/**
 * @brief Генерирует список файлов, как строку и кладёт в буфер
 * @param userPath Путь по которому будет сгенерирован список файлов
 * @param buffer
 * @param bufferSize
 * @return ResponseCode Код ответа см common/include/protocol.h 
 */
ResponseCode generateFileList(const char *userPath, char *buffer, size_t bufferSize);

#endif // GENERATORS_H
