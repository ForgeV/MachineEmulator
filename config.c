#include "config.h"

#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

bool load_configuration(int argc, char *argv[], AppConfig *config) {
    config->speed_operate = 60;
    config->speed_error = 5;
    config->accel_factor = 1;
    config->max_rows = 0;
    config->rotation_strategy = 1;
    config->codepage = CP_UTF8;

    strcpy(config->out_dir_operate, "logs");
    strcpy(config->out_dir_error, "logs");
    strcpy(config->start_datetime, "");
    strcpy(config->time_mask, "YYYY-MM-DD HH:MIN:SS.MMM");

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

    CreateDirectoryA(config->out_dir_operate, NULL);
    
    return true;
}