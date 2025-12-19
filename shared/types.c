#include "types.h"
#include <strings.h>
#include <stddef.h>

/**
 * @file types.c
 * @brief Hàm chuyển đổi/kiểm tra type dùng chung cho client/server.
 */

struct type_entry {
    const char *name;
    enum DeviceType type;
};

/** @brief Bảng mapping từ string -> DeviceType (có alias). */
static const struct type_entry TYPE_TABLE[] = {
    { "drinker", DEVICE_DRINKER },
    { "egg_counter", DEVICE_EGG_COUNTER },
    { "fan", DEVICE_FAN },
    { "feeder", DEVICE_FEEDER },
    { "heater", DEVICE_HEATER },
    { "mist", DEVICE_SPRAYER },        // Alias
    { "mistmaker", DEVICE_SPRAYER },   // Alias
    { "sensor", DEVICE_SENSOR },
    { "sprayer", DEVICE_SPRAYER }
};
static const size_t TYPE_TABLE_SIZE = sizeof(TYPE_TABLE) / sizeof(TYPE_TABLE[0]);

/** @see device_type_to_string() */
const char *device_type_to_string(enum DeviceType type) {
    switch (type) {
    case DEVICE_SENSOR: return "sensor";
    case DEVICE_EGG_COUNTER: return "egg_counter";
    case DEVICE_FEEDER: return "feeder";
    case DEVICE_DRINKER: return "drinker";
    case DEVICE_FAN: return "fan";
    case DEVICE_HEATER: return "heater";
    case DEVICE_SPRAYER: return "sprayer";
    default: return "unknown";
    }
}

/** @see device_type_from_string() */
enum DeviceType device_type_from_string(const char *type_str) {
    // Kiểm tra kỹ hơn
    if (!type_str || type_str[0] == '\0') {
        return DEVICE_UNKNOWN;
    }

    // Linear search (đủ tốt cho ít phần tử)
    for (size_t i = 0; i < TYPE_TABLE_SIZE; ++i) {
        if (strcasecmp(type_str, TYPE_TABLE[i].name) == 0) {
            return TYPE_TABLE[i].type;
        }
    }

    return DEVICE_UNKNOWN;
}
