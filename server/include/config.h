#ifndef CONFIG_H
#define CONFIG_H

/**
 * @brief Грузит конфигурацию из файла массив структур UserConfig
 * Не помню, определились ли мы как будем работать с файлом конфигурации
 * fnmatch - поиск по маске
 * @param configPath Путь к файлу конфига
 * @return int -1 - Ошибка, 0 - Успех
 */
int loadConfig(const char *configPath);

#endif // CONFIG_H
