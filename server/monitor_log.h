#ifndef MONITOR_LOG_H
#define MONITOR_LOG_H

#include "../shared/config.h"

// Hàm ghi log sự kiện (vào file hoặc stdout)
void log_device_event(const char *device_id, const char *message);

#endif  /* MONITOR_LOG_H */
