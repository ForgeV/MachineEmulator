#include "config.h"
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool load_configuration(int argc, char *argv[], AppConfig *config) {

    // Забиваем дефолты. Если конфиг не найдется, программа будет работать на этих числах.
    config->speed_operate = 60;
    config->speed_error = 5;
    config->accel_factor = 1;
    config->max_rows = 0;
    config->rotation_strategy = 1;
    config->codepage = CP_UTF8; // Используем макрос для кодировки.
    strcpy(config->out_dir_operate, "logs");
    strcpy(config->out_dir_error, "logs");
    strcpy(config->start_datetime, "");
    strcpy(config->time_mask, "YYYY-MM-DD HH:MIN:SS.MMM");

    char ini_path[MAX_PATH] = ""; // MAX_PATH — это стандартный лимит на длину пути

    // Проходимся циклом по всем словам, которые ввели при запуске. i=1, так как i=0 — это имя самой программы.
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--config") == 0 && i + 1 < argc) {
            // Превращаем относительный путь в полный.
            GetFullPathNameA(argv[i+1], MAX_PATH, ini_path, NULL);
            break;
        }
    }

    // Если мы нашли путь к конфигу, то читаем его.
    if (strlen(ini_path) > 0) {
        load_from_ini(ini_path, config);
    }

    // Еще один проход по аргументам, чтобы флаги из консоли имели приоритет над конфигом.
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "--speed-op") == 0 && i + 1 < argc) {
            config->speed_operate = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--speed-err") == 0 && i + 1 < argc) {
            config->speed_error = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--accel") == 0 && i + 1 < argc) {
            config->accel_factor = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--max") == 0 && i + 1 < argc) {
            config->max_rows = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--strategy") == 0 && i + 1 < argc) {
            config->rotation_strategy = atoi(argv[++i]);
        } else if (strcmp(argv[i], "--encoding") == 0 && i + 1 < argc) {
            i++;
            // Сверяем текст кодировки с тем, что понимает Windows API.
            if (strcmp(argv[i], "cp1251") == 0) {
                config->codepage = 1251;
            } else if (strcmp(argv[i], "ascii") == 0) {
                config->codepage = 20127;
            } else if (strcmp(argv[i], "utf8") == 0) {
                config->codepage = CP_UTF8;
            } else {
                printf("[WARNING] Неизвестная кодировка '%s', используем UTF-8.\n", argv[i]);
                config->codepage = CP_UTF8;
            }
        } else if (strcmp(argv[i], "--time-mask") == 0 && i + 1 < argc) {
            strncpy(config->time_mask, argv[++i], 63);
        }
    }

    // Создаем папку для логов, если её еще нет. NULL значит с правами доступа по умолчанию.
    CreateDirectoryA(config->out_dir_operate, NULL);

    return true;
}

// Функция для парсинга самого .ini файла.
void load_from_ini(const char* absolute_ini_path, AppConfig* config) {
    // GetPrivateProfileIntA лезет в файл, ищет секцию [Settings] и ключ, возвращает число.
    // Если ключа нет, подставит текущее значение из config.
    config->speed_operate = GetPrivateProfileIntA("Settings", "speed_operate", config->speed_operate, absolute_ini_path);
    config->speed_error = GetPrivateProfileIntA("Settings", "speed_error", config->speed_error, absolute_ini_path);
    config->accel_factor = GetPrivateProfileIntA("Settings", "accel_factor", config->accel_factor, absolute_ini_path);
    config->max_rows = GetPrivateProfileIntA("Settings", "max_rows", config->max_rows, absolute_ini_path);
    config->rotation_strategy = GetPrivateProfileIntA("Settings", "rotation_strategy", config->rotation_strategy, absolute_ini_path);

    // GetPrivateProfileStringA то же самое, но для текста.
    GetPrivateProfileStringA("Settings", "out_dir_operate", config->out_dir_operate, config->out_dir_operate, MAX_PATH, absolute_ini_path);
    GetPrivateProfileStringA("Settings", "out_dir_error", config->out_dir_error, config->out_dir_error, MAX_PATH, absolute_ini_path);
    GetPrivateProfileStringA("Settings", "time_mask", config->time_mask, config->time_mask, 64, absolute_ini_path);

    char encoding_str[32];
    // Читаем кодировку как строку, чтобы потом превратить её в числовой код.
    GetPrivateProfileStringA("Settings", "encoding", "utf8", encoding_str, sizeof(encoding_str), absolute_ini_path);

    if (strcmp(encoding_str, "cp1251") == 0) {
        config->codepage = 1251;
    } else if (strcmp(encoding_str, "ascii") == 0) {
        config->codepage = 20127;
    } else {
        config->codepage = 65001; // UTF-8 по умолчанию
    }
}