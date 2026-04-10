#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "time_utils.h"

static void add_milliseconds(SYSTEMTIME* st, DWORD ms_to_add) {
    FILETIME ft;
    // ULARGE_INTEGER союз, который позволяет
    // смотреть на 64-битное число целиком или как на две 32-битные половинки.
    ULARGE_INTEGER uli;

    // SYSTEMTIME переводим в FILETIME.
    SystemTimeToFileTime(st, &ft);

    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;

    // В FILETIME единица измерения — это 100 наносекунд.
    // Чтобы добавить 1 мс, нужно умножить её на 10 000
    uli.QuadPart += (ULONGLONG)ms_to_add * 10000ULL;

    // Возвращаем результат.
    ft.dwLowDateTime = uli.LowPart;
    ft.dwHighDateTime = uli.HighPart;

    // Переводим из "числа наносекунд" обратно в структуру с датой и временем.
    FileTimeToSystemTime(&ft, st);
}



// Инициализируем контекст времени и решаем с какой скоростью будет работать программа.
bool init_time_context(TimeContext* ctx, const char* start_datetime_str, int items_per_min, int accel_factor) {
    if (items_per_min <= 0 || accel_factor <= 0) return false;

    // Если пользователь не ввел дату старта, берем текущее время компа.
    if (start_datetime_str == NULL || strlen(start_datetime_str) == 0) {
        GetLocalTime(&ctx->current_virtual_time);
    } else {
        GetLocalTime(&ctx->current_virtual_time);
    }

    // Считаем сколько миллисекунд должно пройти в виртуальном времени между логами.
    ctx->virtual_step_ms = 60000 / items_per_min;

    // Сколько поток будет спать в реальности.
    ctx->physical_sleep_ms = ctx->virtual_step_ms / accel_factor;

    // Если шаг слишком маленький, ставим 1 мс, чтобы поток не ушел в бесконечный цикл без сна.
    if (ctx->physical_sleep_ms == 0) {
        ctx->physical_sleep_ms = 1;
    }

    return true;
}

void advance_time(TimeContext* ctx) {
    add_milliseconds(&ctx->current_virtual_time, ctx->virtual_step_ms);
}

// Превращение структуры времени в строку по маске.
bool format_time_string(TimeContext* ctx, const char* format_mask, char* out_buffer, size_t max_len) {
    if (!ctx || !format_mask || !out_buffer) return false;

    SYSTEMTIME* st = &ctx->current_virtual_time;
    char temp[32];

    // Копируем маску в выходной буфер.
    strncpy(out_buffer, format_mask, max_len - 1);
    out_buffer[max_len - 1] = '\0';

    // Год
    snprintf(temp, sizeof(temp), "%04d", st->wYear);
    char* pos;
    while ((pos = strstr(out_buffer, "YYYY")) != NULL) {
        memcpy(pos, temp, 4);
    }

    // Месяц
    snprintf(temp, sizeof(temp), "%02d", st->wMonth);
    while ((pos = strstr(out_buffer, "MM")) != NULL) {
        memcpy(pos, temp, 2);
    }

    // День
    snprintf(temp, sizeof(temp), "%02d", st->wDay);
    while ((pos = strstr(out_buffer, "DD")) != NULL) {
        memcpy(pos, temp, 2);
    }

    // Часы
    snprintf(temp, sizeof(temp), "%02d", st->wHour);
    while ((pos = strstr(out_buffer, "HH")) != NULL) {
        memcpy(pos, temp, 2);
    }

    // Минуты
    snprintf(temp, sizeof(temp), "%02d", st->wMinute);
    while ((pos = strstr(out_buffer, "MIN")) != NULL) {
        memcpy(pos, temp, 2);
    }

    // Секунды
    snprintf(temp, sizeof(temp), "%02d", st->wSecond);
    while ((pos = strstr(out_buffer, "SS")) != NULL) {
        memcpy(pos, temp, 2);
    }

    // Миллисекунды
    snprintf(temp, sizeof(temp), "%03d", st->wMilliseconds);
    while ((pos = strstr(out_buffer, "MS")) != NULL) {
        memcpy(pos, temp, 2);
    }

    return true;
}