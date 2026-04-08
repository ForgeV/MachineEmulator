#ifndef LOGGER_H
#define LOGGER_H

#include <windows.h>
#include <stdbool.h>

typedef struct {
    HANDLE hFile;
    HANDLE hMutex;
    char filepath[MAX_PATH];
    char dir_path[MAX_PATH];
    char base_name[64];
    int rotation_strategy;
    UINT codepage;
} LoggerContext;

bool init_logger(LoggerContext* ctx, const char* dir_path, const char* base_name, int strategy, UINT codepage);
bool write_log(LoggerContext* ctx, const char* log_string);
void close_logger(LoggerContext* ctx);

#endif