#include "devices.h"

#include <jansson.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/**
 * @file devices.c
 * @brief Quản lý danh sách thiết bị và thao tác (INFO/CONTROL/SETCFG).
 *
 * Các hàm trong file này chỉ mô phỏng hành vi thiết bị (in-memory), kèm kiểm
 * tra ràng buộc đơn giản cho tham số cấu hình/hành động.
 */

/** @brief Kiểm tra số nằm trong [min, max]. */
static int in_range(double v, double min, double max) {
    return v >= min && v <= max;
}

enum {
    LIMIT_TEMP_MIN_C = 0,
    LIMIT_TEMP_MAX_C = 60,
    LIMIT_HUM_MIN_PCT = 0,
    LIMIT_HUM_MAX_PCT = 100,
    LIMIT_SPRAY_MAX_LPH = 10,
    LIMIT_FEED_MAX_KG = 10,
    LIMIT_WATER_MAX_L = 20
};

/** @brief Khởi tạo thiết bị cảm biến mặc định. */
static void init_sensor(struct Device *dev, const char *id) {
    memset(dev, 0, sizeof(*dev));
    strncpy(dev->identity.id, id, sizeof(dev->identity.id) - 1);
    dev->identity.type = DEVICE_SENSOR;
    strncpy(dev->password, "123456", sizeof(dev->password) - 1);
    dev->data.sensor.temperature = 32.5;
    dev->data.sensor.humidity = 58.2;
    strncpy(dev->data.sensor.unit_temperature, "C", sizeof(dev->data.sensor.unit_temperature) - 1);
    strncpy(dev->data.sensor.unit_humidity, "%", sizeof(dev->data.sensor.unit_humidity) - 1);
}

/** @brief Khởi tạo thiết bị đếm trứng mặc định. */
static void init_egg_counter(struct Device *dev, const char *id) {
    memset(dev, 0, sizeof(*dev));
    strncpy(dev->identity.id, id, sizeof(dev->identity.id) - 1);
    dev->identity.type = DEVICE_EGG_COUNTER;
    strncpy(dev->password, "123456", sizeof(dev->password) - 1);
    dev->data.egg_counter.egg_count = 35;
}

/** @brief Khởi tạo thiết bị quạt mặc định. */
static void init_fan(struct Device *dev, const char *id) {
    memset(dev, 0, sizeof(*dev));
    strncpy(dev->identity.id, id, sizeof(dev->identity.id) - 1);
    dev->identity.type = DEVICE_FAN;
    strncpy(dev->password, "123456", sizeof(dev->password) - 1);
    dev->data.fan.state = DEVICE_ON;
    dev->data.fan.Tmax = 32.0;
    dev->data.fan.Tp1 = 28.0;
    strncpy(dev->data.fan.unit_temp, "C", sizeof(dev->data.fan.unit_temp) - 1);
}

/** @brief Khởi tạo thiết bị đèn sưởi mặc định. */
static void init_heater(struct Device *dev, const char *id) {
    memset(dev, 0, sizeof(*dev));
    strncpy(dev->identity.id, id, sizeof(dev->identity.id) - 1);
    dev->identity.type = DEVICE_HEATER;
    strncpy(dev->password, "123456", sizeof(dev->password) - 1);
    dev->data.heater.state = DEVICE_OFF;
    dev->data.heater.Tmin = 20.0;
    dev->data.heater.Tp2 = 24.0;
    strncpy(dev->data.heater.mode, "AUTO", sizeof(dev->data.heater.mode) - 1);
    strncpy(dev->data.heater.unit_temp, "C", sizeof(dev->data.heater.unit_temp) - 1);
}

/** @brief Khởi tạo thiết bị phun sương mặc định. */
static void init_sprayer(struct Device *dev, const char *id) {
    memset(dev, 0, sizeof(*dev));
    strncpy(dev->identity.id, id, sizeof(dev->identity.id) - 1);
    dev->identity.type = DEVICE_SPRAYER;
    strncpy(dev->password, "123456", sizeof(dev->password) - 1);
    dev->data.sprayer.state = DEVICE_OFF;
    dev->data.sprayer.Hmin = 45.0;
    dev->data.sprayer.Hp = 60.0;
    dev->data.sprayer.Vh = 0.5;
    strncpy(dev->data.sprayer.unit_humidity, "%", sizeof(dev->data.sprayer.unit_humidity) - 1);
    strncpy(dev->data.sprayer.unit_flow, "L/h", sizeof(dev->data.sprayer.unit_flow) - 1);
}

/** @brief Khởi tạo thiết bị cho ăn mặc định (kèm lịch mẫu). */
static void init_feeder(struct Device *dev, const char *id) {
    memset(dev, 0, sizeof(*dev));
    strncpy(dev->identity.id, id, sizeof(dev->identity.id) - 1);
    dev->identity.type = DEVICE_FEEDER;
    strncpy(dev->password, "123456", sizeof(dev->password) - 1);
    dev->data.feeder.state = DEVICE_ON;
    dev->data.feeder.W = 0.3;
    dev->data.feeder.Vw = 0.5;
    strncpy(dev->data.feeder.unit_food, "kg", sizeof(dev->data.feeder.unit_food) - 1);
    strncpy(dev->data.feeder.unit_water, "L", sizeof(dev->data.feeder.unit_water) - 1);
    dev->data.feeder.schedule_count = 2;
    snprintf(dev->data.feeder.schedule[0].time, sizeof(dev->data.feeder.schedule[0].time), "06:00");
    dev->data.feeder.schedule[0].food = 0.3;
    dev->data.feeder.schedule[0].water = 0.5;
    snprintf(dev->data.feeder.schedule[1].time, sizeof(dev->data.feeder.schedule[1].time), "16:00");
    dev->data.feeder.schedule[1].food = 0.4;
    dev->data.feeder.schedule[1].water = 0.6;
}

/** @brief Khởi tạo thiết bị cho uống mặc định (kèm lịch mẫu). */
static void init_drinker(struct Device *dev, const char *id) {
    memset(dev, 0, sizeof(*dev));
    strncpy(dev->identity.id, id, sizeof(dev->identity.id) - 1);
    dev->identity.type = DEVICE_DRINKER;
    strncpy(dev->password, "123456", sizeof(dev->password) - 1);
    dev->data.drinker.state = DEVICE_ON;
    dev->data.drinker.Vw = 0.4;
    strncpy(dev->data.drinker.unit_water, "L", sizeof(dev->data.drinker.unit_water) - 1);
    dev->data.drinker.schedule_count = 2;
    snprintf(dev->data.drinker.schedule[0].time, sizeof(dev->data.drinker.schedule[0].time), "06:00");
    dev->data.drinker.schedule[0].water = 0.3;
    snprintf(dev->data.drinker.schedule[1].time, sizeof(dev->data.drinker.schedule[1].time), "16:00");
    dev->data.drinker.schedule[1].water = 0.5;
}

void devices_context_init(struct DevicesContext *ctx) {
    if (!ctx) {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
}

size_t devices_scan(const struct DevicesContext *ctx, struct DeviceIdentity *out, size_t max_out) {
    if (!ctx || !out || max_out == 0) {
        return 0;
    }
    size_t copy = ctx->count < max_out ? ctx->count : max_out;
    for (size_t i = 0; i < copy; ++i) {
        out[i] = ctx->devices[i].identity;
    }
    return copy;
}

struct Device *devices_find(struct DevicesContext *ctx, const char *id) {
    if (!ctx || !id) {
        return NULL;
    }
    for (size_t i = 0; i < ctx->count; ++i) {
        if (strncmp(ctx->devices[i].identity.id, id, sizeof(ctx->devices[i].identity.id)) == 0) {
            return &ctx->devices[i];
        }
    }
    return NULL;
}

int devices_change_password(struct Device *dev, const char *old_pw, const char *new_pw) {
    if (!dev || !old_pw || !new_pw) {
        return -1;
    }
    if (strncmp(dev->password, old_pw, sizeof(dev->password)) != 0) {
        return -2;
    }
    strncpy(dev->password, new_pw, sizeof(dev->password) - 1);
    dev->password[sizeof(dev->password) - 1] = '\0';
    return 0;
}

int devices_set_state(struct Device *dev, enum DevicePowerState state) {
    if (!dev) {
        return -1;
    }
    if (state != DEVICE_OFF && state != DEVICE_ON) {
        return -3;
    }
    switch (dev->identity.type) {
    case DEVICE_FAN:
        dev->data.fan.state = state;
        return 0;
    case DEVICE_HEATER:
        dev->data.heater.state = state;
        return 0;
    case DEVICE_SPRAYER:
        dev->data.sprayer.state = state;
        return 0;
    case DEVICE_FEEDER:
        dev->data.feeder.state = state;
        return 0;
    case DEVICE_DRINKER:
        dev->data.drinker.state = state;
        return 0;
    default:
        return -2;
    }
}

int devices_feed_now(struct Device *dev, double food, double water) {
    if (!dev || dev->identity.type != DEVICE_FEEDER) {
        return -1;
    }
    if (!in_range(food, 0.0, (double)LIMIT_FEED_MAX_KG) || !in_range(water, 0.0, (double)LIMIT_WATER_MAX_L)) {
        return -2;
    }
    dev->data.feeder.W = food;
    dev->data.feeder.Vw = water;
    return 0;
}

int devices_drink_now(struct Device *dev, double water) {
    if (!dev || dev->identity.type != DEVICE_DRINKER) {
        return -1;
    }
    if (!in_range(water, 0.0, (double)LIMIT_WATER_MAX_L)) {
        return -2;
    }
    dev->data.drinker.Vw = water;
    return 0;
}

int devices_spray_now(struct Device *dev, double Vh) {
    if (!dev || dev->identity.type != DEVICE_SPRAYER) {
        return -1;
    }
    if (!in_range(Vh, 0.0, (double)LIMIT_SPRAY_MAX_LPH)) {
        return -2;
    }
    dev->data.sprayer.Vh = Vh;
    dev->data.sprayer.state = DEVICE_ON;
    return 0;
}

int devices_set_config_fan(struct Device *dev, double Tmax, double Tp1) {
    if (!dev || dev->identity.type != DEVICE_FAN) {
        return -1;
    }
    if (!in_range(Tmax, (double)LIMIT_TEMP_MIN_C, (double)LIMIT_TEMP_MAX_C) ||
        !in_range(Tp1, (double)LIMIT_TEMP_MIN_C, (double)LIMIT_TEMP_MAX_C) ||
        Tp1 > Tmax) {
        return -2;
    }
    dev->data.fan.Tmax = Tmax;
    dev->data.fan.Tp1 = Tp1;
    return 0;
}

int devices_set_config_heater(struct Device *dev, double Tmin, double Tp2, const char *mode) {
    if (!dev || dev->identity.type != DEVICE_HEATER) {
        return -1;
    }
    if (!in_range(Tmin, (double)LIMIT_TEMP_MIN_C, (double)LIMIT_TEMP_MAX_C) ||
        !in_range(Tp2, (double)LIMIT_TEMP_MIN_C, (double)LIMIT_TEMP_MAX_C) ||
        Tmin > Tp2) {
        return -2;
    }
    dev->data.heater.Tmin = Tmin;
    dev->data.heater.Tp2 = Tp2;
    if (mode && mode[0] != '\0') {
        strncpy(dev->data.heater.mode, mode, sizeof(dev->data.heater.mode) - 1);
        dev->data.heater.mode[sizeof(dev->data.heater.mode) - 1] = '\0';
    }
    return 0;
}

int devices_set_config_sprayer(struct Device *dev, double Hmin, double Hp, double Vh) {
    if (!dev || dev->identity.type != DEVICE_SPRAYER) {
        return -1;
    }
    if (!in_range(Hmin, (double)LIMIT_HUM_MIN_PCT, (double)LIMIT_HUM_MAX_PCT) ||
        !in_range(Hp, (double)LIMIT_HUM_MIN_PCT, (double)LIMIT_HUM_MAX_PCT) ||
        !in_range(Vh, 0.0, (double)LIMIT_SPRAY_MAX_LPH) ||
        Hmin > Hp) {
        return -2;
    }
    dev->data.sprayer.Hmin = Hmin;
    dev->data.sprayer.Hp = Hp;
    dev->data.sprayer.Vh = Vh;
    return 0;
}

/** @brief Giới hạn schedule_count không vượt `MAX_SCHEDULE_ENTRIES`. */
static size_t clamp_schedule_count(size_t count) {
    return count > MAX_SCHEDULE_ENTRIES ? MAX_SCHEDULE_ENTRIES : count;
}

static const char *power_state_string(enum DevicePowerState state) {
    return state == DEVICE_ON ? "ON" : "OFF";
}

static json_t *build_schedule_array(const struct ScheduleEntry *schedule, size_t schedule_count, int include_food) {
    json_t *arr = json_array();
    if (!arr) {
        return NULL;
    }

    size_t count = clamp_schedule_count(schedule_count);
    for (size_t i = 0; i < count; ++i) {
        const struct ScheduleEntry *s = &schedule[i];
        json_t *entry = json_object();
        if (!entry) {
            json_decref(arr);
            return NULL;
        }

        if (json_object_set_new(entry, "time", json_string(s->time)) != 0 ||
            (include_food && json_object_set_new(entry, "food", json_real(s->food)) != 0) ||
            json_object_set_new(entry, "water", json_real(s->water)) != 0 ||
            json_array_append_new(arr, entry) != 0) {
            json_decref(entry);
            json_decref(arr);
            return NULL;
        }
    }

    return arr;
}

int devices_set_config_feeder(struct Device *dev, double W, double Vw, const struct ScheduleEntry *schedule, size_t schedule_count) {
    if (!dev || dev->identity.type != DEVICE_FEEDER) {
        return -1;
    }
    if (!in_range(W, 0.0, (double)LIMIT_FEED_MAX_KG) || !in_range(Vw, 0.0, (double)LIMIT_WATER_MAX_L)) {
        return -2;
    }
    dev->data.feeder.W = W;
    dev->data.feeder.Vw = Vw;
    size_t copy = clamp_schedule_count(schedule_count);
    dev->data.feeder.schedule_count = copy;
    for (size_t i = 0; i < copy; ++i) {
        dev->data.feeder.schedule[i] = schedule[i];
    }
    return 0;
}

int devices_set_config_drinker(struct Device *dev, double Vw, const struct ScheduleEntry *schedule, size_t schedule_count) {
    if (!dev || dev->identity.type != DEVICE_DRINKER) {
        return -1;
    }
    if (!in_range(Vw, 0.0, (double)LIMIT_WATER_MAX_L)) {
        return -2;
    }
    dev->data.drinker.Vw = Vw;
    size_t copy = clamp_schedule_count(schedule_count);
    dev->data.drinker.schedule_count = copy;
    for (size_t i = 0; i < copy; ++i) {
        dev->data.drinker.schedule[i] = schedule[i];
        dev->data.drinker.schedule[i].food = 0.0; /* not used */
    }
    return 0;
}

int devices_info_json(const struct Device *dev, char *out_json, size_t out_len) {
    if (!dev || !out_json || out_len == 0) {
        return -1;
    }

    out_json[0] = '\0';

    int rc = -1;
    char *dumped = NULL;
    json_t *root = json_object();
    if (!root) {
        return -1;
    }

    if (json_object_set_new(root, "device_id", json_string(dev->identity.id)) != 0) {
        goto out;
    }

    switch (dev->identity.type) {
    case DEVICE_SENSOR:
        if (json_object_set_new(root, "type", json_string("sensor")) != 0 ||
            json_object_set_new(root, "temperature", json_real(dev->data.sensor.temperature)) != 0 ||
            json_object_set_new(root, "humidity", json_real(dev->data.sensor.humidity)) != 0 ||
            json_object_set_new(root, "unit_temperature", json_string(dev->data.sensor.unit_temperature)) != 0 ||
            json_object_set_new(root, "unit_humidity", json_string(dev->data.sensor.unit_humidity)) != 0) {
            goto out;
        }
        break;
    case DEVICE_EGG_COUNTER:
        if (json_object_set_new(root, "type", json_string("egg_counter")) != 0 ||
            json_object_set_new(root, "egg_count", json_integer(dev->data.egg_counter.egg_count)) != 0) {
            goto out;
        }
        break;
    case DEVICE_FAN:
        if (json_object_set_new(root, "type", json_string("fan")) != 0 ||
            json_object_set_new(root, "state", json_string(power_state_string(dev->data.fan.state))) != 0 ||
            json_object_set_new(root, "nhiet_do_bat_c", json_real(dev->data.fan.Tmax)) != 0 ||
            json_object_set_new(root, "nhiet_do_tat_c", json_real(dev->data.fan.Tp1)) != 0 ||
            json_object_set_new(root, "unit_temp", json_string(dev->data.fan.unit_temp)) != 0) {
            goto out;
        }
        break;
    case DEVICE_HEATER:
        if (json_object_set_new(root, "type", json_string("heater")) != 0 ||
            json_object_set_new(root, "state", json_string(power_state_string(dev->data.heater.state))) != 0 ||
            json_object_set_new(root, "nhiet_do_bat_c", json_real(dev->data.heater.Tmin)) != 0 ||
            json_object_set_new(root, "nhiet_do_tat_c", json_real(dev->data.heater.Tp2)) != 0 ||
            json_object_set_new(root, "mode", json_string(dev->data.heater.mode)) != 0 ||
            json_object_set_new(root, "unit_temp", json_string(dev->data.heater.unit_temp)) != 0) {
            goto out;
        }
        break;
    case DEVICE_SPRAYER:
        if (json_object_set_new(root, "type", json_string("sprayer")) != 0 ||
            json_object_set_new(root, "state", json_string(power_state_string(dev->data.sprayer.state))) != 0 ||
            json_object_set_new(root, "do_am_bat_pct", json_real(dev->data.sprayer.Hmin)) != 0 ||
            json_object_set_new(root, "do_am_muc_tieu_pct", json_real(dev->data.sprayer.Hp)) != 0 ||
            json_object_set_new(root, "luu_luong_lph", json_real(dev->data.sprayer.Vh)) != 0 ||
            json_object_set_new(root, "unit_humidity", json_string(dev->data.sprayer.unit_humidity)) != 0 ||
            json_object_set_new(root, "unit_flow", json_string(dev->data.sprayer.unit_flow)) != 0) {
            goto out;
        }
        break;
    case DEVICE_FEEDER: {
        json_t *schedule = build_schedule_array(dev->data.feeder.schedule, dev->data.feeder.schedule_count, 1);
        if (!schedule) {
            goto out;
        }
        if (json_object_set_new(root, "type", json_string("feeder")) != 0 ||
            json_object_set_new(root, "state", json_string(power_state_string(dev->data.feeder.state))) != 0 ||
            json_object_set_new(root, "thuc_an_kg", json_real(dev->data.feeder.W)) != 0 ||
            json_object_set_new(root, "nuoc_l", json_real(dev->data.feeder.Vw)) != 0 ||
            json_object_set_new(root, "unit_food", json_string(dev->data.feeder.unit_food)) != 0 ||
            json_object_set_new(root, "unit_water", json_string(dev->data.feeder.unit_water)) != 0 ||
            json_object_set_new(root, "schedule", schedule) != 0) {
            json_decref(schedule);
            goto out;
        }
        break;
    }
    case DEVICE_DRINKER: {
        json_t *schedule = build_schedule_array(dev->data.drinker.schedule, dev->data.drinker.schedule_count, 0);
        if (!schedule) {
            goto out;
        }
        if (json_object_set_new(root, "type", json_string("drinker")) != 0 ||
            json_object_set_new(root, "state", json_string(power_state_string(dev->data.drinker.state))) != 0 ||
            json_object_set_new(root, "nuoc_l", json_real(dev->data.drinker.Vw)) != 0 ||
            json_object_set_new(root, "unit_water", json_string(dev->data.drinker.unit_water)) != 0 ||
            json_object_set_new(root, "schedule", schedule) != 0) {
            json_decref(schedule);
            goto out;
        }
        break;
    }
    default:
        if (json_object_set_new(root, "type", json_string("unknown")) != 0) {
            goto out;
        }
        break;
    }

    dumped = json_dumps(root, JSON_COMPACT | JSON_REAL_PRECISION(4));
    if (!dumped) {
        goto out;
    }
    size_t dumped_len = strlen(dumped);
    if (dumped_len + 1 > out_len) {
        goto out;
    }
    memcpy(out_json, dumped, dumped_len + 1);
    rc = 0;

out:
    free(dumped);
    json_decref(root);
    return rc;
}

void devices_init_default_device(struct Device *dev, enum DeviceType type, const char *id, const char *password) {
    switch (type) {
    case DEVICE_SENSOR:
        init_sensor(dev, id);
        break;
    case DEVICE_EGG_COUNTER:
        init_egg_counter(dev, id);
        break;
    case DEVICE_FAN:
        init_fan(dev, id);
        break;
    case DEVICE_HEATER:
        init_heater(dev, id);
        break;
    case DEVICE_SPRAYER:
        init_sprayer(dev, id);
        break;
    case DEVICE_FEEDER:
        init_feeder(dev, id);
        break;
    case DEVICE_DRINKER:
        init_drinker(dev, id);
        break;
    default:
        memset(dev, 0, sizeof(*dev));
        strncpy(dev->identity.id, id, sizeof(dev->identity.id) - 1);
        dev->identity.type = DEVICE_UNKNOWN;
        strncpy(dev->password, "123456", sizeof(dev->password) - 1);
        return;
    }
    if (password && password[0] != '\0') {
        strncpy(dev->password, password, sizeof(dev->password) - 1);
        dev->password[sizeof(dev->password) - 1] = '\0';
    }
}

int devices_add(struct DevicesContext *ctx, const char *id, enum DeviceType type, const char *password, int coop_id) {
    if (!ctx || !id || id[0] == '\0') {
        return -1;
    }
    if (coop_id <= 0) {
        return -1;
    }
    if (ctx->count >= MAX_DEVICES) {
        return -2;
    }
    if (devices_find(ctx, id) != NULL) {
        return -3; /* da ton tai */
    }
    struct Device *dev = &ctx->devices[ctx->count];
    memset(dev, 0, sizeof(*dev));
    devices_init_default_device(dev, type, id, password ? password : "123456");
    dev->identity.coop_id = coop_id;
    ctx->count++;
    return 0;
}
