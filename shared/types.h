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

/** @brief Trạng thái thiết bị (phục vụ hiển thị, không phải mọi type đều dùng). */
enum DeviceStatus {
    STATUS_OFF = 0,
    STATUS_ON,
    STATUS_AUTO,
    STATUS_ERROR
};

/** @brief Định danh thiết bị trong hệ thống. */
struct DeviceIdentity {
    char id[MAX_ID_LEN];
    enum DeviceType type;
    int coop_id; /* 0 = chua gan chuong */
};

/** @brief Thông tin mở rộng (tuỳ chọn) về thiết bị. */
struct DeviceInfo {
    struct DeviceIdentity identity;
    enum DeviceStatus status;
    char description[64];  // Mô tả ngắn
    char location[64];     // Vị trí lắp đặt
    time_t last_updated;   // Thời gian cập nhật cuối
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

/** @brief Chuyển `DeviceStatus` sang chuỗi (vd "ON", "OFF", ...). */
const char *device_status_to_string(enum DeviceStatus status);

/** @brief Parse chuỗi status sang `DeviceStatus` (không phân biệt hoa/thường). */
enum DeviceStatus device_status_from_string(const char *status_str);

#endif  /* SHARED_TYPES_H */
