#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <stdbool.h>
#include <time.h>
#include "logger.h"

#define FIVE_MB (5 * 1024 * 1024)

// Структура для "асинхронного сброса". Нужна, чтобы перекинуть данные
// из оперативной памяти в файл в отдельном потоке и не тормозить основной эмулятор.
typedef struct {
    char* buffer;           // Указатель на кусок памяти с логами
    DWORD buffer_size;      // DWORD это unsigned int
    char archive_filepath[MAX_PATH]; // Куда сохранять
} AsyncFlushData;

// Генерируем имя файла с таймстампом.
void generate_archive_name(const char* dir_path, const char* base_name, char* out_filepath) {
    SYSTEMTIME st;
    GetLocalTime(&st); // Берем системное время через WinAPI
    sprintf(out_filepath, "%s\\%s_%04d%02d%02d_%02d%02d%02d.log",
            dir_path, base_name, st.wYear, st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
}

// Функция, которая выполняется в отдельном потоке
DWORD WINAPI FlushBufferThread(LPVOID lpParam) {
    // lpParam универсальный указатель
    AsyncFlushData* data = (AsyncFlushData*)lpParam;

    // Спим 3 сек, имитируя нагрузку или просто чтобы не забивать диск мгновенно
    Sleep(3000);

    // Создаем файл архива
    HANDLE hArchive = CreateFileA(data->archive_filepath, GENERIC_WRITE, 0, NULL, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);
    if (hArchive != INVALID_HANDLE_VALUE) {
        DWORD bytesWritten;
        WriteFile(hArchive, data->buffer, data->buffer_size, &bytesWritten, NULL);
        FlushFileBuffers(hArchive); // Гарантируем, что данные реально упали на диск, а не зависли в кэше
        CloseHandle(hArchive);
    }

    // Освобождаем память, которую выделили в основном потоке.
    free(data->buffer);
    free(data);
    return 0;
}

// Настройка логгера при старте
bool init_logger(LoggerContext* ctx, const char* dir_path, const char* base_name, int strategy, UINT codepage) {
    strcpy(ctx->dir_path, dir_path);
    strcpy(ctx->base_name, base_name);
    sprintf(ctx->filepath, "%s\\%s.log", dir_path, base_name);
    ctx->rotation_strategy = strategy;

    // Создаем МЬЮТЕКС.
    ctx->hMutex = CreateMutex(NULL, FALSE, NULL);

    // Открываем основной файл лога. OPEN_ALWAYS — если нет, создаст, если есть — откроет.
    ctx->hFile = CreateFileA(ctx->filepath, FILE_APPEND_DATA | GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    ctx->codepage = codepage;
    return (ctx->hFile != INVALID_HANDLE_VALUE && ctx->hMutex != NULL);
}

// Самая главная функция записи
bool write_log(LoggerContext* ctx, const char* log_string) {
    if (!ctx || ctx->hFile == INVALID_HANDLE_VALUE) return false;

    // Ждем своей очереди на захват мьютекса.
    WaitForSingleObject(ctx->hMutex, INFINITE);

    // Проверяем размер файла.
    LARGE_INTEGER fileSize;
    GetFileSizeEx(ctx->hFile, &fileSize);

    if (fileSize.QuadPart >= FIVE_MB) {
        char archive_path[MAX_PATH];
        generate_archive_name(ctx->dir_path, ctx->base_name, archive_path);

        // Стратегия 1: Просто переименовать файл и создать новый
        if (ctx->rotation_strategy == 1) {
            CloseHandle(ctx->hFile); // Закрываем старый
            MoveFileExA(ctx->filepath, archive_path, MOVEFILE_REPLACE_EXISTING); // Переименовываем
            ctx->hFile = CreateFileA(ctx->filepath, FILE_APPEND_DATA | GENERIC_READ, FILE_SHARE_READ, NULL,
                                     CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL); // Открываем чистый
        }
        // Стратегия 2: Скопировать содержимое в оперативную память и очистить файл.
        else if (ctx->rotation_strategy == 2) {
            char* buffer = (char*)malloc(fileSize.LowPart);
            if (buffer) {
                DWORD bytesRead;
                SetFilePointer(ctx->hFile, 0, NULL, FILE_BEGIN); // Прыгаем в начало файла
                ReadFile(ctx->hFile, buffer, fileSize.LowPart, &bytesRead, NULL); // Читаем всё в оперативной памяти

                SetFilePointer(ctx->hFile, 0, NULL, FILE_BEGIN);
                SetEndOfFile(ctx->hFile); // Очищаем файл

                // Собираем данные для фонового потока
                AsyncFlushData* async_data = (AsyncFlushData*)malloc(sizeof(AsyncFlushData));
                async_data->buffer = buffer;
                async_data->buffer_size = bytesRead;
                strcpy(async_data->archive_filepath, archive_path);

                // Запускаем поток, который сохранит старые логи в фоне
                CreateThread(NULL, 0, FlushBufferThread, async_data, 0, NULL);
            }
        }
    }
    // Работа с кодировками.
    const char* data_to_write = log_string;
    char converted_buffer[1024];
    DWORD data_len = (DWORD)strlen(log_string);

    // Если кодировка не UTF-8, конвертируем через WideChar.
    if (ctx->codepage != CP_UTF8) {
        wchar_t wbuf[1024]; // Буфер для UTF-16
        int wlen = MultiByteToWideChar(CP_UTF8, 0, log_string, -1, wbuf, 1024);
        if (wlen > 0) {
            // Из UTF-16 перегоняем в ту кодировку, которую попросил пользователь.
            int clen = WideCharToMultiByte(ctx->codepage, 0, wbuf, -1, converted_buffer, 1024, NULL, NULL);
            if (clen > 0) {
                data_to_write = converted_buffer;
                data_len = clen - 1; // Убираем нуль-терминатор
            }
        }
    }

    // Прыгаем в самый конец файла, чтобы дописать, а не перезаписать
    SetFilePointer(ctx->hFile, 0, NULL, FILE_END);
    DWORD bytesWritten;

    // Пишем саму строку
    WriteFile(ctx->hFile, data_to_write, data_len, &bytesWritten, NULL);
    // Дописываем перенос строки
    WriteFile(ctx->hFile, "\r\n", 2, &bytesWritten, NULL);

    // Сбрасываем кэш на диск, чтобы логи не пропали в случае вылета
    FlushFileBuffers(ctx->hFile);

    // Отпускаем мьютекс
    ReleaseMutex(ctx->hMutex);

    return true;
}

// Закрываем всё при завершении программы
void close_logger(LoggerContext* ctx) {
    if (ctx->hFile != INVALID_HANDLE_VALUE) {
        CloseHandle(ctx->hFile);
    }
    if (ctx->hMutex) {
        CloseHandle(ctx->hMutex); // Удаляем мьютекс из системы
    }
}