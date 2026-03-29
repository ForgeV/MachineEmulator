#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>
#include <windows.h>

typedef struct {
    int speed_operate;
    int speed_error;
    int accel_factor;
    int max_rows;
    int rotation_strategy;
    char out_dir_operate[MAX_PATH];
    char out_dir_error[MAX_PATH];
    char start_datetime[32];
    UINT codepage;
    char time_mask[64];
} AppConfig;

bool load_configuration(int argc, char *argv[], AppConfig *config);

#endif