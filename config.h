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
    char path_source_op[MAX_PATH];
    char path_source_err[MAX_PATH];
} AppConfig;

extern volatile BOOL fullDiskFlag;
extern volatile long long int totalBytes;

bool load_configuration(int argc, char *argv[], AppConfig *config);
void load_from_ini(const char* absolute_ini_path, AppConfig* config);
#endif