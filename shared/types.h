#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H
#include <stddef.h>
#include <time.h>
#include "config.h"

enum DeviceType {
    DEVICE_SENSOR = 0,
    DEVICE_EGG_COUNTER,
    DEVICE_FEEDER,
    DEVICE_DRINKER,
    DEVICE_FAN,
    DEVICE_HEATER,
    DEVICE_SPRAYER,
    DEVICE_UNKNOWN
};

// Validation macro
#define DEVICE_TYPE_IS_VALID(t) ((t) >= DEVICE_SENSOR && (t) <= DEVICE_SPRAYER)

// Device status
enum DeviceStatus {
    STATUS_OFF = 0,
    STATUS_ON,
    STATUS_AUTO,
    STATUS_ERROR
};

struct DeviceIdentity {
    char id[MAX_ID_LEN];
    enum DeviceType type;
};

// Extended device information (optional)
struct DeviceInfo {
    struct DeviceIdentity identity;
    enum DeviceStatus status;
    char description[64];  // Mô tả ngắn
    char location[64];     // Vị trí lắp đặt
    time_t last_updated;   // Thời gian cập nhật cuối
};

// Basic conversion functions
const char *device_type_to_string(enum DeviceType type);
enum DeviceType device_type_from_string(const char *type_str);

// Status conversion functions (optional)
const char *device_status_to_string(enum DeviceStatus status);
enum DeviceStatus device_status_from_string(const char *status_str);

// Validation function
int device_type_is_valid(enum DeviceType type);

#endif  /* SHARED_TYPES_H */
