#include <stdbool.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"
#include "logger.h"
#include "generator.h"
#include "time_utils.h"
#include "worker.h"

extern HANDLE hShutdownEvent;
extern HANDLE hConsoleMutex;
volatile long long int totalBytes = 0;
// CreateThread принимает только один указатель, поэтому мы упаковываем всё нужное в эту структуру.
typedef struct {
    AppConfig* config;
    TemplateList* templates;
    LoggerContext* logger;
    const char* thread_name;
    bool is_error_thread;
} WorkerParams;

// Сердце эмулятора. Общая логика для любого типа потока.
DWORD WINAPI base_worker(LPVOID lpParam) {
    // Превращаем void* обратно в структуру с данными.
    WorkerParams* p = (WorkerParams*)lpParam;
    TimeContext time_ctx;

    // Выбираем скорость: либо для ошибок, либо для обычной работы.
    int speed = p->is_error_thread ? p->config->speed_error : p->config->speed_operate;

    // Инициализируем время для этого потока.
    init_time_context(&time_ctx, p->config->start_datetime, speed, p->config->accel_factor);

    char log_body[512];
    char time_str[64];
    char final_line[1024];
    int rows_generated = 0;

    while (WaitForSingleObject(hShutdownEvent, 0) == WAIT_TIMEOUT) {

        // Проверяем по лимиту строк.
        if (p->config->max_rows > 0 && rows_generated >= p->config->max_rows) {
            // Захватываем мьютекс, чтобы наше сообщение в консоль не перебило другой поток.
            WaitForSingleObject(hConsoleMutex, INFINITE);
            printf("[%s] Достигнут лимит строк (%d). Поток завершается.\n", p->thread_name, p->config->max_rows);
            ReleaseMutex(hConsoleMutex);
            break;
        }
        if (fullDiskFlag) {
            WaitForSingleObject(hConsoleMutex, INFINITE);
            printf("[%s] Достигнут лимит по размеру диска (%d). Поток завершается.\n", p->thread_name, p->config->max_rows);
            ReleaseMutex(hConsoleMutex);
            break;
        }

        // 1. Генерируем случайную строку по шаблону.
        if (generate_event_string(p->templates, log_body, sizeof(log_body))) {

            // 2. Получаем текущее виртуальное время в виде строки.
            format_time_string(&time_ctx, p->config->time_mask, time_str, sizeof(time_str));

            // 3. Склеиваем время и тело лога в одну строчку.
            snprintf(final_line, sizeof(final_line), "[%s] %s", time_str, log_body);

            totalBytes += sizeof(char) * strlen(final_line);

            // 4. Отправляем в логгер.
            write_log(p->logger, final_line);

            rows_generated++;
        }

        // Двигаем виртуальные часы вперед.
        advance_time(&time_ctx);

        // Спим в реальном времени.
        Sleep(time_ctx.physical_sleep_ms);
    }
    return 0;
}

// Поток для штатной работы станка.
DWORD WINAPI operate_worker(LPVOID lpParam) {
    AppConfig* config = (AppConfig*)lpParam;

    TemplateList templates;
    LoggerContext logger;

    // Загружаем шаблоны.
    load_templates(config->path_source_op, &templates);

    // Инициализируем логгер для файла operate.log.
    init_logger(&logger, config->out_dir_operate, "operate", config->rotation_strategy, config->codepage);

    // Собираем параметры и запускаем базовую функцию.
    WorkerParams params = { config, &templates, &logger, "OPERATE", false };
    base_worker(&params);

    // Очистка
    close_logger(&logger);
    free_templates(&templates);
    return 0;
}

// Поток для генерации ошибок. Делает всё то же самое, но с другими файлами.
DWORD WINAPI error_worker(LPVOID lpParam) {
    AppConfig* config = (AppConfig*)lpParam;

    TemplateList templates;
    LoggerContext logger;

    load_templates(config->path_source_err, &templates);
    init_logger(&logger, config->out_dir_error, "error", config->rotation_strategy, config->codepage);

    WorkerParams params = { config, &templates, &logger, "ERROR", true };
    base_worker(&params);

    close_logger(&logger);
    free_templates(&templates);
    return 0;
}