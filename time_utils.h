#ifndef TIME_UTILS_H
#define TIME_UTILS_H

#include <windows.h>
#include <stdbool.h>

typedef struct {
    SYSTEMTIME current_virtual_time;
    DWORD virtual_step_ms;
    DWORD physical_sleep_ms;
} TimeContext;

bool init_time_context(TimeContext* ctx, const char* start_datetime_str, int items_per_min, int accel_factor);
void advance_time(TimeContext* ctx);
bool format_time_string(TimeContext* ctx, const char* format_mask, char* out_buffer, size_t max_len);

#endif