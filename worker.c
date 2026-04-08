#include <stdbool.h>
#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include "config.h"      // Предполагаемые заголовки
#include "logger.h"
#include "generator.h"
#include "time_utils.h"
#include "worker.h"
extern HANDLE hShutdownEvent;
extern HANDLE hConsoleMutex;

typedef struct {
    AppConfig* config;
    TemplateList* templates;
    LoggerContext* logger;
    const char* thread_name;
    bool is_error_thread;
} WorkerParams;

DWORD WINAPI base_worker(LPVOID lpParam) {
    WorkerParams* p = (WorkerParams*)lpParam;
    TimeContext time_ctx;

    int speed = p->is_error_thread ? p->config->speed_error : p->config->speed_operate;
    init_time_context(&time_ctx, p->config->start_datetime, speed, p->config->accel_factor);

    char log_body[512];
    char time_str[64];
    char final_line[1024];
    int rows_generated = 0;

    while (WaitForSingleObject(hShutdownEvent, 0) == WAIT_TIMEOUT) {
        if (p->config->max_rows > 0 && rows_generated >= p->config->max_rows) {
            WaitForSingleObject(hConsoleMutex, INFINITE);
            printf("[%s] Достигнут лимит строк (%d). Поток завершается.\n", p->thread_name, p->config->max_rows);
            ReleaseMutex(hConsoleMutex);
            break;
        }
        if (generate_event_string(p->templates, log_body, sizeof(log_body))) {
            format_time_string(&time_ctx, p->config->time_mask, time_str, sizeof(time_str));

            snprintf(final_line, sizeof(final_line), "[%s] %s", time_str, log_body);

            write_log(p->logger, final_line);
            
            rows_generated++;
        }
        advance_time(&time_ctx);
        Sleep(time_ctx.physical_sleep_ms);
    }
    return 0;
}

DWORD WINAPI operate_worker(LPVOID lpParam) {
    AppConfig* config = (AppConfig*)lpParam;

    TemplateList templates;
    LoggerContext logger;
    
    load_templates("../source/source_operate.txt", &templates);
    init_logger(&logger, config->out_dir_operate, "operate", config->rotation_strategy,config->codepage);

    WorkerParams params = { config, &templates, &logger, "OPERATE", false };
    
    base_worker(&params);

    close_logger(&logger);
    free_templates(&templates);
    return 0;
}

DWORD WINAPI error_worker(LPVOID lpParam) {
    AppConfig* config = (AppConfig*)lpParam;
    
    TemplateList templates;
    LoggerContext logger;
    
    load_templates("../source/source_error.txt", &templates);
    init_logger(&logger, config->out_dir_error, "error", config->rotation_strategy, config->codepage);

    WorkerParams params = { config, &templates, &logger, "ERROR", true };
    
    base_worker(&params);

    close_logger(&logger);
    free_templates(&templates);
    return 0;
}