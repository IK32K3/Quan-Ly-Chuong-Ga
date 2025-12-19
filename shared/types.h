#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H
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

/** @brief Định danh thiết bị trong hệ thống. */
struct DeviceIdentity {
    char id[MAX_ID_LEN];
    enum DeviceType type;
    int coop_id; /* 0 = chua gan chuong */
};

/**
 * @brief Chuyển `DeviceType` sang chuỗi chuẩn dùng trong protocol/JSON.
 * @return Chuỗi hằng (vd "fan", "sensor", ...); "unknown" nếu không biết.
 */
const char *device_type_to_string(enum DeviceType type);

/**
 * @brief Parse chuỗi type sang `DeviceType` (không phân biệt hoa/thường).
 * @return `DEVICE_UNKNOWN` nếu không parse được.
 */
enum DeviceType device_type_from_string(const char *type_str);

#endif  /* SHARED_TYPES_H */
