#include "types.h"

#include <strings.h>

struct type_entry {
    const char *name;
    enum DeviceType type;
};

static const struct type_entry TYPE_TABLE[] = {
    { "sensor", DEVICE_SENSOR },
    { "egg_counter", DEVICE_EGG_COUNTER },
    { "feeder", DEVICE_FEEDER },
    { "drinker", DEVICE_DRINKER },
    { "fan", DEVICE_FAN },
    { "heater", DEVICE_HEATER },
    { "sprayer", DEVICE_SPRAYER },
    { "mist", DEVICE_SPRAYER },
    { "mistmaker", DEVICE_SPRAYER }
};

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

enum DeviceType device_type_from_string(const char *type_str) {
    if (!type_str) {
        return DEVICE_UNKNOWN;
    }

    for (size_t i = 0; i < sizeof(TYPE_TABLE) / sizeof(TYPE_TABLE[0]); ++i) {
        if (strcasecmp(type_str, TYPE_TABLE[i].name) == 0) {
            return TYPE_TABLE[i].type;
        }
    }

    return DEVICE_UNKNOWN;
}
