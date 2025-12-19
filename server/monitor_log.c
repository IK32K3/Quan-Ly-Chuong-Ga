#include "monitor_log.h"
#include <stdio.h>
#include <time.h>

/**
 * @file monitor_log.c
 * @brief Ghi log sự kiện thiết bị ra file.
 */

/** @see log_device_event() */
void log_device_event(const char *device_id, const char *message) {
    FILE *log_file = fopen("device_log.txt", "a");
    if (!log_file) return;

    time_t now = time(NULL);
    struct tm *tm_info = localtime(&now);
    char time_str[20];
    strftime(time_str, sizeof(time_str), "%Y-%m-%d %H:%M:%S", tm_info);

    fprintf(log_file, "[%s] Device %s: %s\n", time_str, device_id, message);
    fclose(log_file);
}
