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
    if (!ctx || !format_mask || !out_buffer || max_len == 0) return false;

    SYSTEMTIME* st = &ctx->current_virtual_time;
    const char* m = format_mask;
    size_t out_idx = 0;

    // Идем по маске, пока не достигнем конца
    while (*m != '\0' && out_idx < max_len - 1) {
        size_t space_left = max_len - out_idx;
        int w = 0;

        // Проверка тегов
        if (strncmp(m, "yyyy", 4) == 0 || strncmp(m, "YYYY", 4) == 0)  {
            w = snprintf(out_buffer + out_idx, space_left, "%04d", st->wYear);
            if (w > 0 && (size_t)w < space_left) { out_idx += w; m += 4; continue; }
        }
        else if (strncmp(m, "yy", 2) == 0 || strncmp(m, "YY", 2) == 0) {
            w = snprintf(out_buffer + out_idx, space_left, "%02d", st->wYear % 100);
            if (w > 0 && (size_t)w < space_left) { out_idx += w; m += 2; continue; }
        }
        else if (strncmp(m, "SSS", 3) == 0 || strncmp(m, "sss", 3) == 0) {
            w = snprintf(out_buffer + out_idx, space_left, "%03d", st->wMilliseconds);
            if (w > 0 && (size_t)w < space_left) { out_idx += w; m += 3; continue; }
        }
        else if (strncmp(m, "MM", 2) == 0) {
            w = snprintf(out_buffer + out_idx, space_left, "%02d", st->wMonth);
            if (w > 0 && (size_t)w < space_left) { out_idx += w; m += 2; continue; }
        }
        else if (strncmp(m, "dd", 2) == 0 || strncmp(m, "DD", 2) == 0) {
            w = snprintf(out_buffer + out_idx, space_left, "%02d", st->wDay);
            if (w > 0 && (size_t)w < space_left) { out_idx += w; m += 2; continue; }
        }
        else if (strncmp(m, "HH", 2) == 0 || strncmp(m, "hh", 2) == 0) {
            w = snprintf(out_buffer + out_idx, space_left, "%02d", st->wHour);
            if (w > 0 && (size_t)w < space_left) { out_idx += w; m += 2; continue; }
        }
        else if (strncmp(m, "mm", 2) == 0) {
            w = snprintf(out_buffer + out_idx, space_left, "%02d", st->wMinute);
            if (w > 0 && (size_t)w < space_left) { out_idx += w; m += 2; continue; }
        }
        else if (strncmp(m, "ss", 2) == 0 || strncmp(m, "SS", 2) == 0) {
            w = snprintf(out_buffer + out_idx, space_left, "%02d", st->wSecond);
            if (w > 0 && (size_t)w < space_left) { out_idx += w; m += 2; continue; }
        }

        // Если это не тег, копируем как есть
        out_buffer[out_idx++] = *m++;
    }
    out_buffer[out_idx] = '\0';

    return (*m == '\0');
}