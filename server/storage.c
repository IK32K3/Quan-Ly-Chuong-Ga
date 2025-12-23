#include "storage.h"
#include <jansson.h>

#include <stdio.h>
#include <string.h>

static void copy_string(char *dst, size_t dst_len, const char *src) {
    if (!dst || dst_len == 0) return;
    if (!src) {
        dst[0] = '\0';
        return;
    }
    strncpy(dst, src, dst_len - 1);
    dst[dst_len - 1] = '\0';
}

static enum DevicePowerState parse_state_or_default(const char *state, enum DevicePowerState def) {
    if (!state) return def;
    /* Project luon ghi "ON"/"OFF" (in hoa), nen khong can so sanh khong phan biet hoa/thuong. */
    return (strcmp(state, "ON") == 0) ? DEVICE_ON : DEVICE_OFF;
}

static size_t clamp_schedule_count(size_t count) {
    return (count > MAX_SCHEDULE_ENTRIES) ? MAX_SCHEDULE_ENTRIES : count;
}

static int parse_device_info_object(struct Device *dev, json_t *info) {
    if (!dev || !json_is_object(info)) return -1;

    json_t *type_val = json_object_get(info, "type");
    json_t *id_val = json_object_get(info, "device_id");
    if (!json_is_string(type_val) || !json_is_string(id_val)) {
        return -1;
    }

    const char *type_str = json_string_value(type_val);
    const char *id = json_string_value(id_val);
    if (!type_str || !id || id[0] == '\0') {
        return -1;
    }

    copy_string(dev->identity.id, sizeof(dev->identity.id), id);
    dev->identity.type = device_type_from_string(type_str);

    switch (dev->identity.type) {
    case DEVICE_SENSOR: {
        json_t *temperature = json_object_get(info, "temperature");
        json_t *humidity = json_object_get(info, "humidity");
        if (json_is_number(temperature)) dev->data.sensor.temperature = json_number_value(temperature);
        if (json_is_number(humidity)) dev->data.sensor.humidity = json_number_value(humidity);

        json_t *ut = json_object_get(info, "unit_temperature");
        json_t *uh = json_object_get(info, "unit_humidity");
        if (json_is_string(ut)) copy_string(dev->data.sensor.unit_temperature, sizeof(dev->data.sensor.unit_temperature), json_string_value(ut));
        if (json_is_string(uh)) copy_string(dev->data.sensor.unit_humidity, sizeof(dev->data.sensor.unit_humidity), json_string_value(uh));
        break;
    }
    case DEVICE_EGG_COUNTER: {
        json_t *eggs = json_object_get(info, "egg_count");
        if (json_is_integer(eggs)) dev->data.egg_counter.egg_count = (int)json_integer_value(eggs);
        else if (json_is_number(eggs)) dev->data.egg_counter.egg_count = (int)json_number_value(eggs);
        break;
    }
    case DEVICE_FAN: {
        json_t *tmax = json_object_get(info, "nhiet_do_bat_c");
        json_t *tp1 = json_object_get(info, "nhiet_do_tat_c");
        if (json_is_number(tmax)) dev->data.fan.Tmax = json_number_value(tmax);
        if (json_is_number(tp1)) dev->data.fan.Tp1 = json_number_value(tp1);

        json_t *unit = json_object_get(info, "unit_temp");
        if (json_is_string(unit)) copy_string(dev->data.fan.unit_temp, sizeof(dev->data.fan.unit_temp), json_string_value(unit));

        json_t *state = json_object_get(info, "state");
        dev->data.fan.state = parse_state_or_default(json_is_string(state) ? json_string_value(state) : NULL, DEVICE_OFF);
        break;
    }
    case DEVICE_HEATER: {
        json_t *tmin = json_object_get(info, "nhiet_do_bat_c");
        json_t *tp2 = json_object_get(info, "nhiet_do_tat_c");
        if (json_is_number(tmin)) dev->data.heater.Tmin = json_number_value(tmin);
        if (json_is_number(tp2)) dev->data.heater.Tp2 = json_number_value(tp2);

        json_t *mode = json_object_get(info, "mode");
        if (json_is_string(mode)) copy_string(dev->data.heater.mode, sizeof(dev->data.heater.mode), json_string_value(mode));

        json_t *unit = json_object_get(info, "unit_temp");
        if (json_is_string(unit)) copy_string(dev->data.heater.unit_temp, sizeof(dev->data.heater.unit_temp), json_string_value(unit));

        json_t *state = json_object_get(info, "state");
        dev->data.heater.state = parse_state_or_default(json_is_string(state) ? json_string_value(state) : NULL, DEVICE_OFF);
        break;
    }
    case DEVICE_SPRAYER: {
        json_t *hmin = json_object_get(info, "do_am_bat_pct");
        json_t *hp = json_object_get(info, "do_am_muc_tieu_pct");
        json_t *vh = json_object_get(info, "luu_luong_lph");
        if (json_is_number(hmin)) dev->data.sprayer.Hmin = json_number_value(hmin);
        if (json_is_number(hp)) dev->data.sprayer.Hp = json_number_value(hp);
        if (json_is_number(vh)) dev->data.sprayer.Vh = json_number_value(vh);

        json_t *uh = json_object_get(info, "unit_humidity");
        json_t *uf = json_object_get(info, "unit_flow");
        if (json_is_string(uh)) copy_string(dev->data.sprayer.unit_humidity, sizeof(dev->data.sprayer.unit_humidity), json_string_value(uh));
        if (json_is_string(uf)) copy_string(dev->data.sprayer.unit_flow, sizeof(dev->data.sprayer.unit_flow), json_string_value(uf));

        json_t *state = json_object_get(info, "state");
        dev->data.sprayer.state = parse_state_or_default(json_is_string(state) ? json_string_value(state) : NULL, DEVICE_OFF);
        break;
    }
    case DEVICE_FEEDER: {
        json_t *w = json_object_get(info, "thuc_an_kg");
        json_t *vw = json_object_get(info, "nuoc_l");
        if (json_is_number(w)) dev->data.feeder.W = json_number_value(w);
        if (json_is_number(vw)) dev->data.feeder.Vw = json_number_value(vw);

        json_t *uf = json_object_get(info, "unit_food");
        json_t *uw = json_object_get(info, "unit_water");
        if (json_is_string(uf)) copy_string(dev->data.feeder.unit_food, sizeof(dev->data.feeder.unit_food), json_string_value(uf));
        if (json_is_string(uw)) copy_string(dev->data.feeder.unit_water, sizeof(dev->data.feeder.unit_water), json_string_value(uw));

        json_t *state = json_object_get(info, "state");
        dev->data.feeder.state = parse_state_or_default(json_is_string(state) ? json_string_value(state) : NULL, DEVICE_OFF);

        json_t *schedule = json_object_get(info, "schedule");
        size_t n = (json_is_array(schedule)) ? clamp_schedule_count(json_array_size(schedule)) : 0;
        dev->data.feeder.schedule_count = n;
        for (size_t i = 0; i < n; ++i) {
            json_t *e = json_array_get(schedule, i);
            if (!json_is_object(e)) continue;

            json_t *time = json_object_get(e, "time");
            if (json_is_string(time)) copy_string(dev->data.feeder.schedule[i].time, sizeof(dev->data.feeder.schedule[i].time), json_string_value(time));

            json_t *food = json_object_get(e, "food");
            json_t *water = json_object_get(e, "water");
            if (json_is_number(food)) dev->data.feeder.schedule[i].food = json_number_value(food);
            if (json_is_number(water)) dev->data.feeder.schedule[i].water = json_number_value(water);
        }
        break;
    }
    case DEVICE_DRINKER: {
        json_t *vw = json_object_get(info, "nuoc_l");
        if (json_is_number(vw)) dev->data.drinker.Vw = json_number_value(vw);

        json_t *uw = json_object_get(info, "unit_water");
        if (json_is_string(uw)) copy_string(dev->data.drinker.unit_water, sizeof(dev->data.drinker.unit_water), json_string_value(uw));

        json_t *state = json_object_get(info, "state");
        dev->data.drinker.state = parse_state_or_default(json_is_string(state) ? json_string_value(state) : NULL, DEVICE_OFF);

        json_t *schedule = json_object_get(info, "schedule");
        size_t n = (json_is_array(schedule)) ? clamp_schedule_count(json_array_size(schedule)) : 0;
        dev->data.drinker.schedule_count = n;
        for (size_t i = 0; i < n; ++i) {
            json_t *e = json_array_get(schedule, i);
            if (!json_is_object(e)) continue;

            json_t *time = json_object_get(e, "time");
            if (json_is_string(time)) copy_string(dev->data.drinker.schedule[i].time, sizeof(dev->data.drinker.schedule[i].time), json_string_value(time));

            json_t *water = json_object_get(e, "water");
            if (json_is_number(water)) dev->data.drinker.schedule[i].water = json_number_value(water);
            dev->data.drinker.schedule[i].food = 0.0;
        }
        break;
    }
    default:
        break;
    }

    return 0;
}

static json_t *build_device_info_value(const struct Device *dev) {
    if (!dev) return NULL;

    json_t *info = json_object();
    if (!info) return NULL;

    if (json_object_set_new(info, "device_id", json_string(dev->identity.id)) != 0 ||
        json_object_set_new(info, "type", json_string(device_type_to_string(dev->identity.type))) != 0) {
        json_decref(info);
        return NULL;
    }

    switch (dev->identity.type) {
    case DEVICE_SENSOR:
        (void)json_object_set_new(info, "temperature", json_real(dev->data.sensor.temperature));
        (void)json_object_set_new(info, "humidity", json_real(dev->data.sensor.humidity));
        (void)json_object_set_new(info, "unit_temperature", json_string(dev->data.sensor.unit_temperature));
        (void)json_object_set_new(info, "unit_humidity", json_string(dev->data.sensor.unit_humidity));
        break;
    case DEVICE_EGG_COUNTER:
        (void)json_object_set_new(info, "egg_count", json_integer(dev->data.egg_counter.egg_count));
        break;
    case DEVICE_FAN:
        (void)json_object_set_new(info, "state", json_string(dev->data.fan.state == DEVICE_ON ? "ON" : "OFF"));
        (void)json_object_set_new(info, "nhiet_do_bat_c", json_real(dev->data.fan.Tmax));
        (void)json_object_set_new(info, "nhiet_do_tat_c", json_real(dev->data.fan.Tp1));
        (void)json_object_set_new(info, "unit_temp", json_string(dev->data.fan.unit_temp));
        break;
    case DEVICE_HEATER:
        (void)json_object_set_new(info, "state", json_string(dev->data.heater.state == DEVICE_ON ? "ON" : "OFF"));
        (void)json_object_set_new(info, "nhiet_do_bat_c", json_real(dev->data.heater.Tmin));
        (void)json_object_set_new(info, "nhiet_do_tat_c", json_real(dev->data.heater.Tp2));
        (void)json_object_set_new(info, "mode", json_string(dev->data.heater.mode));
        (void)json_object_set_new(info, "unit_temp", json_string(dev->data.heater.unit_temp));
        break;
    case DEVICE_SPRAYER:
        (void)json_object_set_new(info, "state", json_string(dev->data.sprayer.state == DEVICE_ON ? "ON" : "OFF"));
        (void)json_object_set_new(info, "do_am_bat_pct", json_real(dev->data.sprayer.Hmin));
        (void)json_object_set_new(info, "do_am_muc_tieu_pct", json_real(dev->data.sprayer.Hp));
        (void)json_object_set_new(info, "luu_luong_lph", json_real(dev->data.sprayer.Vh));
        (void)json_object_set_new(info, "unit_humidity", json_string(dev->data.sprayer.unit_humidity));
        (void)json_object_set_new(info, "unit_flow", json_string(dev->data.sprayer.unit_flow));
        break;
    case DEVICE_FEEDER: {
        (void)json_object_set_new(info, "state", json_string(dev->data.feeder.state == DEVICE_ON ? "ON" : "OFF"));
        (void)json_object_set_new(info, "thuc_an_kg", json_real(dev->data.feeder.W));
        (void)json_object_set_new(info, "nuoc_l", json_real(dev->data.feeder.Vw));
        (void)json_object_set_new(info, "unit_food", json_string(dev->data.feeder.unit_food));
        (void)json_object_set_new(info, "unit_water", json_string(dev->data.feeder.unit_water));

        json_t *sched = json_array();
        size_t n = clamp_schedule_count(dev->data.feeder.schedule_count);
        for (size_t i = 0; i < n; ++i) {
            const struct ScheduleEntry *s = &dev->data.feeder.schedule[i];
            json_t *e = json_object();
            (void)json_object_set_new(e, "time", json_string(s->time));
            (void)json_object_set_new(e, "food", json_real(s->food));
            (void)json_object_set_new(e, "water", json_real(s->water));
            (void)json_array_append_new(sched, e);
        }
        (void)json_object_set_new(info, "schedule", sched);
        break;
    }
    case DEVICE_DRINKER: {
        (void)json_object_set_new(info, "state", json_string(dev->data.drinker.state == DEVICE_ON ? "ON" : "OFF"));
        (void)json_object_set_new(info, "nuoc_l", json_real(dev->data.drinker.Vw));
        (void)json_object_set_new(info, "unit_water", json_string(dev->data.drinker.unit_water));

        json_t *sched = json_array();
        size_t n = clamp_schedule_count(dev->data.drinker.schedule_count);
        for (size_t i = 0; i < n; ++i) {
            const struct ScheduleEntry *s = &dev->data.drinker.schedule[i];
            json_t *e = json_object();
            (void)json_object_set_new(e, "time", json_string(s->time));
            (void)json_object_set_new(e, "water", json_real(s->water));
            (void)json_array_append_new(sched, e);
        }
        (void)json_object_set_new(info, "schedule", sched);
        break;
    }
    default:
        break;
    }

    return info;
}

int storage_save_farm(const struct CoopsContext *coops, const struct DevicesContext *devices, const char *path) {
    if (!coops || !devices || !path) return -1;

    json_t *root = json_object();
    if (!root) return -1;
    json_t *coops_arr = json_array();
    if (!coops_arr) {
        json_decref(root);
        return -1;
    }
    (void)json_object_set_new(root, "coops", coops_arr);

    for (size_t i = 0; i < coops->count; ++i) {
        const struct CoopMeta *c = &coops->coops[i];

        json_t *coop = json_object();
        (void)json_object_set_new(coop, "id", json_integer(c->id));
        (void)json_object_set_new(coop, "name", json_string(c->name));

        json_t *devs_arr = json_array();
        (void)json_object_set_new(coop, "devices", devs_arr);

        for (size_t j = 0; j < devices->count; ++j) {
            const struct Device *d = &devices->devices[j];
            if (d->identity.coop_id != c->id) continue;

            json_t *entry = json_object();
            (void)json_object_set_new(entry, "password", json_string(d->password));

            json_t *info_val = build_device_info_value(d);
            if (!info_val) {
                json_decref(entry);
                continue;
            }
            (void)json_object_set_new(entry, "info", info_val);
            (void)json_array_append_new(devs_arr, entry);
        }

        (void)json_array_append_new(coops_arr, coop);
    }

    int rc = json_dump_file(root, path, JSON_INDENT(2)) == 0 ? 0 : -1;
    json_decref(root);
    return rc;
}

int storage_load_farm(struct CoopsContext *coops, struct DevicesContext *devices, const char *path) {
    if (!coops || !devices || !path) return -1;

    json_t *root = json_load_file(path, 0, NULL);
    if (!root) return -1;
    if (!json_is_object(root)) {
        json_decref(root);
        return -1;
    }

    json_t *coops_arr = json_object_get(root, "coops");
    if (!json_is_array(coops_arr)) {
        json_decref(root);
        return -1;
    }

    coops_init(coops);
    memset(devices, 0, sizeof(*devices));

    size_t coop_count = json_array_size(coops_arr);
    for (size_t i = 0; i < coop_count; ++i) {
        json_t *coop = json_array_get(coops_arr, i);
        if (!json_is_object(coop)) continue;

        json_t *id_val = json_object_get(coop, "id");
        if (!json_is_integer(id_val)) continue;
        int coop_id = (int)json_integer_value(id_val);
        if (coop_id <= 0) continue;

        const char *name = NULL;
        json_t *name_val = json_object_get(coop, "name");
        if (json_is_string(name_val)) name = json_string_value(name_val);
        char coop_name[MAX_COOP_NAME];
        if (!name || name[0] == '\0' || strcmp(name, "0") == 0) {
            snprintf(coop_name, sizeof(coop_name), "Chuong %d", coop_id);
        } else {
            copy_string(coop_name, sizeof(coop_name), name);
        }
        (void)coops_upsert(coops, coop_id, coop_name);

        json_t *devs_arr = json_object_get(coop, "devices");
        if (!json_is_array(devs_arr)) continue;

        size_t dev_count = json_array_size(devs_arr);
        for (size_t j = 0; j < dev_count && devices->count < MAX_DEVICES; ++j) {
            json_t *entry = json_array_get(devs_arr, j);
            if (!json_is_object(entry)) continue;

            json_t *info = json_object_get(entry, "info");
            if (!json_is_object(info)) continue;

            struct Device dev_tmp;
            memset(&dev_tmp, 0, sizeof(dev_tmp));
            dev_tmp.identity.coop_id = coop_id;
            json_t *pw_val = json_object_get(entry, "password");
            if (json_is_string(pw_val)) {
                copy_string(dev_tmp.password, sizeof(dev_tmp.password), json_string_value(pw_val));
            }
            if (dev_tmp.password[0] == '\0') {
                copy_string(dev_tmp.password, sizeof(dev_tmp.password), "123456");
            }
            if (parse_device_info_object(&dev_tmp, info) != 0) {
                continue;
            }
            if (devices_find(devices, dev_tmp.identity.id) != NULL) {
                continue;
            }
            devices->devices[devices->count++] = dev_tmp;
        }
    }

    json_decref(root);
    return 0;
}
