#include <windows.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// =====================================================================
// Структуры данных (В финале будут в time_utils.h)
// =====================================================================
#include "time_utils.h"

static void add_milliseconds(SYSTEMTIME* st, DWORD ms_to_add) {
    FILETIME ft;
    ULARGE_INTEGER uli;

    SystemTimeToFileTime(st, &ft);

    uli.LowPart = ft.dwLowDateTime;
    uli.HighPart = ft.dwHighDateTime;
    uli.QuadPart += (ULONGLONG)ms_to_add * 10000ULL;

    ft.dwLowDateTime = uli.LowPart;
    ft.dwHighDateTime = uli.HighPart;
    FileTimeToSystemTime(&ft, st);
}

bool init_time_context(TimeContext* ctx, const char* start_datetime_str, int items_per_min, int accel_factor) {
    if (items_per_min <= 0 || accel_factor <= 0) return false;

    if (start_datetime_str == NULL || strlen(start_datetime_str) == 0) {
        GetLocalTime(&ctx->current_virtual_time);
    } else {
        // добавить парсинг
        GetLocalTime(&ctx->current_virtual_time);
    }
    ctx->virtual_step_ms = 60000 / items_per_min;
    ctx->physical_sleep_ms = ctx->virtual_step_ms / accel_factor;

    if (ctx->physical_sleep_ms == 0) {
        ctx->physical_sleep_ms = 1; 
    }

    return true;
}

void advance_time(TimeContext* ctx) {
    add_milliseconds(&ctx->current_virtual_time, ctx->virtual_step_ms);
}

bool format_time_string(TimeContext* ctx, const char* format_mask, char* out_buffer, size_t max_len) {
    if (!ctx || !format_mask || !out_buffer) return false;

    SYSTEMTIME* st = &ctx->current_virtual_time;
    char temp[32];

    strncpy(out_buffer, format_mask, max_len - 1);
    out_buffer[max_len - 1] = '\0';

    snprintf(temp, sizeof(temp), "%04d", st->wYear);
    char* pos;
    while ((pos = strstr(out_buffer, "YYYY")) != NULL) {
        memcpy(pos, temp, 4);
    }

    snprintf(temp, sizeof(temp), "%02d", st->wMonth);
    while ((pos = strstr(out_buffer, "MM")) != NULL) {
        memcpy(pos, temp, 2);
    }

    snprintf(temp, sizeof(temp), "%02d", st->wDay);
    while ((pos = strstr(out_buffer, "DD")) != NULL) {
        memcpy(pos, temp, 2);
    }

    snprintf(temp, sizeof(temp), "%02d", st->wHour);
    while ((pos = strstr(out_buffer, "HH")) != NULL) {
        memcpy(pos, temp, 2);
    }

    snprintf(temp, sizeof(temp), "%02d", st->wMinute);
    while ((pos = strstr(out_buffer, "MIN")) != NULL) {
        memcpy(pos, temp, 3);
    }

    snprintf(temp, sizeof(temp), "%02d", st->wSecond);
    while ((pos = strstr(out_buffer, "SS")) != NULL) {
        memcpy(pos, temp, 2);
    }

    snprintf(temp, sizeof(temp), "%03d", st->wMilliseconds);
    while ((pos = strstr(out_buffer, "MS")) != NULL) {
        memcpy(pos, temp, 2);
    }
    return true;
}