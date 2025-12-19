#include "devices.h"

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

/** @brief Kiểm tra số hữu hạn (cách đơn giản: NaN != NaN). */
static int is_finite_number(double v) {
    return v == v;
}

/** @brief Kiểm tra số nằm trong [min, max] và không phải NaN. */
static int in_range(double v, double min, double max) {
    return is_finite_number(v) && v >= min && v <= max;
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

/** @see devices_context_init() */
void devices_context_init(struct DevicesContext *ctx) {
    if (!ctx) {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
    if (ctx->count < MAX_DEVICES) {
        init_sensor(&ctx->devices[ctx->count++], "SENSOR1");
    }
    if (ctx->count < MAX_DEVICES) {
        init_fan(&ctx->devices[ctx->count++], "FAN1");
    }
    if (ctx->count < MAX_DEVICES) {
        init_heater(&ctx->devices[ctx->count++], "HEAT1");
    }
    if (ctx->count < MAX_DEVICES) {
        init_sprayer(&ctx->devices[ctx->count++], "MIST1");
    }
    if (ctx->count < MAX_DEVICES) {
        init_feeder(&ctx->devices[ctx->count++], "FEED1");
    }
    if (ctx->count < MAX_DEVICES) {
        init_egg_counter(&ctx->devices[ctx->count++], "EGG1");
    }
    if (ctx->count < MAX_DEVICES) {
        init_drinker(&ctx->devices[ctx->count++], "DRINK1");
    }

    /* Mac dinh: tat ca thiet bi nam trong chuong 1 */
    for (size_t i = 0; i < ctx->count; ++i) {
        if (ctx->devices[i].identity.coop_id <= 0) {
            ctx->devices[i].identity.coop_id = 1;
        }
    }
}

/** @see devices_scan() */
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

/** @see devices_find() */
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

/** @see devices_find_const() */
const struct Device *devices_find_const(const struct DevicesContext *ctx, const char *id) {
    return devices_find((struct DevicesContext *)ctx, id);
}

/** @see devices_change_password() */
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

/** @see devices_set_state() */
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

/** @see devices_feed_now() */
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

/** @see devices_drink_now() */
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

/** @see devices_spray_now() */
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

/** @see devices_set_config_fan() */
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

/** @see devices_set_config_heater() */
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

/** @see devices_set_config_sprayer() */
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

/** @see devices_set_config_feeder() */
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

/** @see devices_set_config_drinker() */
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

/** @see devices_info_json() */
int devices_info_json(const struct Device *dev, char *out_json, size_t out_len) {
    if (!dev || !out_json || out_len == 0) {
        return -1;
    }

    int written = 0;
    switch (dev->identity.type) {
    case DEVICE_SENSOR:
        written = snprintf(out_json, out_len,
            "{\"device_id\":\"%s\",\"type\":\"sensor\",\"temperature\":%.1f,\"humidity\":%.1f,"
            "\"unit_temperature\":\"%s\",\"unit_humidity\":\"%s\"}",
            dev->identity.id,
            dev->data.sensor.temperature,
            dev->data.sensor.humidity,
            dev->data.sensor.unit_temperature,
            dev->data.sensor.unit_humidity);
        break;
    case DEVICE_EGG_COUNTER:
        written = snprintf(out_json, out_len,
            "{\"device_id\":\"%s\",\"type\":\"egg_counter\",\"egg_count\":%d}",
            dev->identity.id,
            dev->data.egg_counter.egg_count);
        break;
    case DEVICE_FAN:
        written = snprintf(out_json, out_len,
            "{\"device_id\":\"%s\",\"type\":\"fan\",\"state\":\"%s\",\"Tmax\":%.1f,\"Tp1\":%.1f,"
            "\"unit_temp\":\"%s\"}",
            dev->identity.id,
            dev->data.fan.state == DEVICE_ON ? "ON" : "OFF",
            dev->data.fan.Tmax,
            dev->data.fan.Tp1,
            dev->data.fan.unit_temp);
        break;
    case DEVICE_HEATER:
        written = snprintf(out_json, out_len,
            "{\"device_id\":\"%s\",\"type\":\"heater\",\"state\":\"%s\",\"Tmin\":%.1f,\"Tp2\":%.1f,"
            "\"mode\":\"%s\",\"unit_temp\":\"%s\"}",
            dev->identity.id,
            dev->data.heater.state == DEVICE_ON ? "ON" : "OFF",
            dev->data.heater.Tmin,
            dev->data.heater.Tp2,
            dev->data.heater.mode,
            dev->data.heater.unit_temp);
        break;
    case DEVICE_SPRAYER:
        written = snprintf(out_json, out_len,
            "{\"device_id\":\"%s\",\"type\":\"sprayer\",\"state\":\"%s\",\"Hmin\":%.1f,\"Hp\":%.1f,"
            "\"Vh\":%.1f,\"unit_humidity\":\"%s\",\"unit_flow\":\"%s\"}",
            dev->identity.id,
            dev->data.sprayer.state == DEVICE_ON ? "ON" : "OFF",
            dev->data.sprayer.Hmin,
            dev->data.sprayer.Hp,
            dev->data.sprayer.Vh,
            dev->data.sprayer.unit_humidity,
            dev->data.sprayer.unit_flow);
        break;
    case DEVICE_FEEDER: {
        /* build schedule array manually */
        char schedule_buf[MAX_JSON_LEN];
        size_t pos = 0;
        pos += snprintf(schedule_buf + pos, sizeof(schedule_buf) - pos, "[");
        for (size_t i = 0; i < dev->data.feeder.schedule_count; ++i) {
            const struct ScheduleEntry *s = &dev->data.feeder.schedule[i];
            pos += snprintf(schedule_buf + pos, sizeof(schedule_buf) - pos,
                "%s{\"time\":\"%s\",\"food\":%.1f,\"water\":%.1f}",
                (i == 0 ? "" : ","),
                s->time,
                s->food,
                s->water);
            if (pos >= sizeof(schedule_buf)) {
                break;
            }
        }
        snprintf(schedule_buf + pos, sizeof(schedule_buf) - pos, "]");
        written = snprintf(out_json, out_len,
            "{\"device_id\":\"%s\",\"type\":\"feeder\",\"state\":\"%s\",\"W\":%.1f,\"Vw\":%.1f,"
            "\"unit_food\":\"%s\",\"unit_water\":\"%s\",\"schedule\":%s}",
            dev->identity.id,
            dev->data.feeder.state == DEVICE_ON ? "ON" : "OFF",
            dev->data.feeder.W,
            dev->data.feeder.Vw,
            dev->data.feeder.unit_food,
            dev->data.feeder.unit_water,
            schedule_buf);
        break;
    }
    case DEVICE_DRINKER: {
        char schedule_buf[MAX_JSON_LEN];
        size_t pos = 0;
        pos += snprintf(schedule_buf + pos, sizeof(schedule_buf) - pos, "[");
        for (size_t i = 0; i < dev->data.drinker.schedule_count; ++i) {
            const struct ScheduleEntry *s = &dev->data.drinker.schedule[i];
            pos += snprintf(schedule_buf + pos, sizeof(schedule_buf) - pos,
                "%s{\"time\":\"%s\",\"water\":%.1f}",
                (i == 0 ? "" : ","),
                s->time,
                s->water);
            if (pos >= sizeof(schedule_buf)) {
                break;
            }
        }
        snprintf(schedule_buf + pos, sizeof(schedule_buf) - pos, "]");
        written = snprintf(out_json, out_len,
            "{\"device_id\":\"%s\",\"type\":\"drinker\",\"state\":\"%s\",\"Vw\":%.1f,"
            "\"unit_water\":\"%s\",\"schedule\":%s}",
            dev->identity.id,
            dev->data.drinker.state == DEVICE_ON ? "ON" : "OFF",
            dev->data.drinker.Vw,
            dev->data.drinker.unit_water,
            schedule_buf);
        break;
    }
    default:
        written = snprintf(out_json, out_len,
            "{\"device_id\":\"%s\",\"type\":\"unknown\"}",
            dev->identity.id);
        break;
    }

    if (written < 0 || (size_t)written >= out_len) {
        return -1;
    }
    return 0;
}

/** @see devices_init_default_device() */
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

/** @see devices_add() */
int devices_add(struct DevicesContext *ctx, const char *id, enum DeviceType type, const char *password, int coop_id) {
    if (!ctx || !id || id[0] == '\0') {
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
    dev->identity.coop_id = (coop_id <= 0) ? 1 : coop_id;
    ctx->count++;
    return 0;
}
