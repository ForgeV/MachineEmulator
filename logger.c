#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "logger.h"
#define FIVE_MB (5 * 1024 * 1024)

typedef struct {
    char* buffer;
    DWORD buffer_size;
    char archive_filepath[MAX_PATH];
} AsyncFlushData;

void generate_archive_name(const char* dir_path, const char* base_name, char* out_filepath) {
    SYSTEMTIME st;
    GetLocalTime(&st);
    sprintf(out_filepath, "%s\\%s_%04d%02d%02d_%02d%02d%02d.log",
            dir_path, base_name, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

DWORD WINAPI FlushBufferThread(LPVOID lpParam) {
    AsyncFlushData* data = (AsyncFlushData*)lpParam;

    Sleep(5000);

    HANDLE hArchive = CreateFileA(data->archive_filepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hArchive != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        WriteFile(hArchive, data->buffer, data->buffer_size, &bytesWritten, NULL);
        FlushFileBuffers(hArchive);
        CloseHandle(hArchive);
    }

    free(data->buffer);
    free(data);
    return 0;
}

bool init_logger(LoggerContext* ctx, const char* dir_path, const char* base_name, int strategy, UINT codepage) {
    strcpy(ctx->dir_path, dir_path);
    strcpy(ctx->base_name, base_name);
    sprintf(ctx->filepath, "%s\\%s.log", dir_path, base_name);
    ctx->rotation_strategy = strategy;

    ctx->hMutex = CreateMutex(NULL, FALSE, NULL);

    ctx->hFile = CreateFileA(ctx->filepath, FILE_APPEND_DATA | GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    ctx->codepage = codepage;
    return (ctx->hFile != INVALID_HANDLE_VALUE && ctx->hMutex != NULL);
}

bool write_log(LoggerContext* ctx, const char* log_string) {
    if (!ctx || ctx->hFile == INVALID_HANDLE_VALUE) return false;
    WaitForSingleObject(ctx->hMutex, INFINITE);

    LARGE_INTEGER fileSize;
    GetFileSizeEx(ctx->hFile, &fileSize);

    if (fileSize.QuadPart >= FIVE_MB) {
        char archive_path[MAX_PATH];
        generate_archive_name(ctx->dir_path, ctx->base_name, archive_path);

        if (ctx->rotation_strategy == 1) {
            CloseHandle(ctx->hFile);

            MoveFileExA(ctx->filepath, archive_path, MOVEFILE_REPLACE_EXISTING);

            ctx->hFile = CreateFileA(ctx->filepath, FILE_APPEND_DATA | GENERIC_READ, FILE_SHARE_READ, NULL,
                                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
        }
        else if (ctx->rotation_strategy == 2) {
            char* buffer = (char*)malloc(fileSize.LowPart);
            if (buffer) {
                DWORD bytesRead;

                SetFilePointer(ctx->hFile, 0, NULL, FILE_BEGIN);
                ReadFile(ctx->hFile, buffer, fileSize.LowPart, &bytesRead, NULL);

                SetFilePointer(ctx->hFile, 0, NULL, FILE_BEGIN);
                SetEndOfFile(ctx->hFile);

                AsyncFlushData* async_data = (AsyncFlushData*)malloc(sizeof(AsyncFlushData));
                async_data->buffer = buffer;
                async_data->buffer_size = bytesRead;
                strcpy(async_data->archive_filepath, archive_path);

                CreateThread(NULL, 0, FlushBufferThread, async_data, 0, NULL);
            }
        }
    }

    const char* data_to_write = log_string;
    char converted_buffer[1024];
    DWORD data_len = (DWORD)strlen(log_string);

    if (ctx->codepage != CP_UTF8) {
        wchar_t wbuf[1024];

        int wlen = MultiByteToWideChar(CP_UTF8, 0, log_string, -1, wbuf, 1024);
        if (wlen > 0) {
            int clen = WideCharToMultiByte(ctx->codepage, 0, wbuf, -1, converted_buffer, 1024, NULL, NULL);
            if (clen > 0) {
                data_to_write = converted_buffer;
                data_len = clen - 1;
            }
        }
    }
    SetFilePointer(ctx->hFile, 0, NULL, FILE_END);
    DWORD bytesWritten;

    WriteFile(ctx->hFile, data_to_write, data_len, &bytesWritten, NULL);
    WriteFile(ctx->hFile, "\r\n", 2, &bytesWritten, NULL);
    FlushFileBuffers(ctx->hFile);

    ReleaseMutex(ctx->hMutex);

    return true;
}

void close_logger(LoggerContext* ctx) {
    if (ctx->hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(ctx->hFile);
    }
    if (ctx->hMutex) {
        CloseHandle(ctx->hMutex);
    }
}