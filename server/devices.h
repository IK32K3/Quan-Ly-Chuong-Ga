#ifndef SERVER_DEVICES_H
#define SERVER_DEVICES_H

#include <stddef.h>
#include <stdint.h>
#include "../shared/config.h"
#include "../shared/types.h"

#define MAX_SCHEDULE_ENTRIES 8

enum DevicePowerState {
    DEVICE_OFF = 0,
    DEVICE_ON = 1
};

struct ScheduleEntry {
    char time[6];   /* HH:MM */
    double food;    /* kg */
    double water;   /* L */
};

struct SensorData {
    double temperature;
    double humidity;
    char unit_temperature[4];
    char unit_humidity[4];
};

struct EggCounterData {
    int egg_count;
};

struct FeederData {
    enum DevicePowerState state;
    double W;   /* kg per meal */
    double Vw;  /* L per meal */
    char unit_food[8];
    char unit_water[8];
    struct ScheduleEntry schedule[MAX_SCHEDULE_ENTRIES];
    size_t schedule_count;
};

struct DrinkerData {
    enum DevicePowerState state;
    double Vw;  /* L per meal */
    char unit_water[8];
    struct ScheduleEntry schedule[MAX_SCHEDULE_ENTRIES];
    size_t schedule_count;
};

struct FanData {
    enum DevicePowerState state;
    double Tmax;
    double Tp1;
    char unit_temp[4];
};

struct HeaterData {
    enum DevicePowerState state;
    double Tmin;
    double Tp2;
    char mode[8];       /* e.g., AUTO/MANUAL */
    char unit_temp[4];
};

struct SprayerData {
    enum DevicePowerState state;
    double Hmin;
    double Hp;
    double Vh;
    char unit_humidity[4];
    char unit_flow[8];  /* L/h */
};

union DeviceData {
    struct SensorData sensor;
    struct EggCounterData egg_counter;
    struct FeederData feeder;
    struct DrinkerData drinker;
    struct FanData fan;
    struct HeaterData heater;
    struct SprayerData sprayer;
};

struct Device {
    struct DeviceIdentity identity;
    char password[MAX_PASSWORD_LEN];
    union DeviceData data;
};

struct DevicesContext {
    struct Device devices[MAX_DEVICES];
    size_t count;
};

void devices_context_init(struct DevicesContext *ctx);
size_t devices_scan(const struct DevicesContext *ctx, struct DeviceIdentity *out, size_t max_out);
struct Device *devices_find(struct DevicesContext *ctx, const char *id);
const struct Device *devices_find_const(const struct DevicesContext *ctx, const char *id);

int devices_change_password(struct Device *dev, const char *old_pw, const char *new_pw);

int devices_set_state(struct Device *dev, enum DevicePowerState state);
int devices_feed_now(struct Device *dev, double food, double water);
int devices_drink_now(struct Device *dev, double water);
int devices_spray_now(struct Device *dev, double Vh);

int devices_set_config_fan(struct Device *dev, double Tmax, double Tp1);
int devices_set_config_heater(struct Device *dev, double Tmin, double Tp2, const char *mode);
int devices_set_config_sprayer(struct Device *dev, double Hmin, double Hp, double Vh);
int devices_set_config_feeder(struct Device *dev, double W, double Vw, const struct ScheduleEntry *schedule, size_t schedule_count);
int devices_set_config_drinker(struct Device *dev, double Vw, const struct ScheduleEntry *schedule, size_t schedule_count);

int devices_info_json(const struct Device *dev, char *out_json, size_t out_len);

/* Them thiet bi moi voi cau hinh mac dinh theo type */
int devices_add(struct DevicesContext *ctx, const char *id, enum DeviceType type, const char *password);

#endif /* SERVER_DEVICES_H */
