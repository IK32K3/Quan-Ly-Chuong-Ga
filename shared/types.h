#ifndef SHARED_TYPES_H
#define SHARED_TYPES_H

#include <stddef.h>
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

struct DeviceIdentity {
    char id[MAX_ID_LEN];
    enum DeviceType type;
};

const char *device_type_to_string(enum DeviceType type);
enum DeviceType device_type_from_string(const char *type_str);

#endif  /* SHARED_TYPES_H */
