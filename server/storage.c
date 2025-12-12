#include "storage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

/* Helpers parse JSON don gian */
static const char *find_key(const char *json, const char *key) {
    if (!json || !key) return NULL;
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    return strstr(json, pattern);
}

static int json_get_string(const char *json, const char *key, char *out, size_t out_len) {
    const char *p = find_key(json, key);
    if (!p) return -1;
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return -1;
    p++;
    size_t idx = 0;
    while (*p && *p != '"' && idx + 1 < out_len) {
        out[idx++] = *p++;
    }
    out[idx] = '\0';
    return 0;
}

static int json_get_double(const char *json, const char *key, double *out) {
    const char *p = find_key(json, key);
    if (!p) return -1;
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (sscanf(p, "%lf", out) == 1) {
        return 0;
    }
    return -1;
}

static void parse_device_info(struct Device *dev, const char *info_json) {
    if (!dev || !info_json) return;
    char type[32];
    char id[MAX_ID_LEN];
    if (json_get_string(info_json, "type", type, sizeof(type)) != 0 ||
        json_get_string(info_json, "device_id", id, sizeof(id)) != 0) {
        return;
    }
    strncpy(dev->identity.id, id, sizeof(dev->identity.id) - 1);
    dev->identity.type = device_type_from_string(type);
    switch (dev->identity.type) {
    case DEVICE_SENSOR:
        json_get_double(info_json, "temperature", &dev->data.sensor.temperature);
        json_get_double(info_json, "humidity", &dev->data.sensor.humidity);
        json_get_string(info_json, "unit_temperature", dev->data.sensor.unit_temperature, sizeof(dev->data.sensor.unit_temperature));
        json_get_string(info_json, "unit_humidity", dev->data.sensor.unit_humidity, sizeof(dev->data.sensor.unit_humidity));
        break;
    case DEVICE_EGG_COUNTER: {
        double eggs = 0.0;
        json_get_double(info_json, "egg_count", &eggs);
        dev->data.egg_counter.egg_count = (int)eggs;
        break;
    }
    case DEVICE_FAN:
        json_get_double(info_json, "Tmax", &dev->data.fan.Tmax);
        json_get_double(info_json, "Tp1", &dev->data.fan.Tp1);
        json_get_string(info_json, "unit_temp", dev->data.fan.unit_temp, sizeof(dev->data.fan.unit_temp));
        if (find_key(info_json, "\"state\":\"ON\"")) dev->data.fan.state = DEVICE_ON;
        else dev->data.fan.state = DEVICE_OFF;
        break;
    case DEVICE_HEATER:
        json_get_double(info_json, "Tmin", &dev->data.heater.Tmin);
        json_get_double(info_json, "Tp2", &dev->data.heater.Tp2);
        json_get_string(info_json, "mode", dev->data.heater.mode, sizeof(dev->data.heater.mode));
        json_get_string(info_json, "unit_temp", dev->data.heater.unit_temp, sizeof(dev->data.heater.unit_temp));
        if (find_key(info_json, "\"state\":\"ON\"")) dev->data.heater.state = DEVICE_ON;
        else dev->data.heater.state = DEVICE_OFF;
        break;
    case DEVICE_SPRAYER:
        json_get_double(info_json, "Hmin", &dev->data.sprayer.Hmin);
        json_get_double(info_json, "Hp", &dev->data.sprayer.Hp);
        json_get_double(info_json, "Vh", &dev->data.sprayer.Vh);
        json_get_string(info_json, "unit_humidity", dev->data.sprayer.unit_humidity, sizeof(dev->data.sprayer.unit_humidity));
        json_get_string(info_json, "unit_flow", dev->data.sprayer.unit_flow, sizeof(dev->data.sprayer.unit_flow));
        if (find_key(info_json, "\"state\":\"ON\"")) dev->data.sprayer.state = DEVICE_ON;
        else dev->data.sprayer.state = DEVICE_OFF;
        break;
    case DEVICE_FEEDER:
        json_get_double(info_json, "W", &dev->data.feeder.W);
        json_get_double(info_json, "Vw", &dev->data.feeder.Vw);
        json_get_string(info_json, "unit_food", dev->data.feeder.unit_food, sizeof(dev->data.feeder.unit_food));
        json_get_string(info_json, "unit_water", dev->data.feeder.unit_water, sizeof(dev->data.feeder.unit_water));
        if (find_key(info_json, "\"state\":\"ON\"")) dev->data.feeder.state = DEVICE_ON;
        else dev->data.feeder.state = DEVICE_OFF;
        break;
    case DEVICE_DRINKER:
        json_get_double(info_json, "Vw", &dev->data.drinker.Vw);
        json_get_string(info_json, "unit_water", dev->data.drinker.unit_water, sizeof(dev->data.drinker.unit_water));
        if (find_key(info_json, "\"state\":\"ON\"")) dev->data.drinker.state = DEVICE_ON;
        else dev->data.drinker.state = DEVICE_OFF;
        break;
    default:
        break;
    }
}

int storage_save_devices(const struct DevicesContext *ctx, const char *path) {
    if (!ctx || !path) return -1;
    FILE *f = fopen(path, "w");
    if (!f) return -1;
    fprintf(f, "[\n");
    for (size_t i = 0; i < ctx->count; ++i) {
        char info[MAX_JSON_LEN];
        devices_info_json(&ctx->devices[i], info, sizeof(info));
        fprintf(f, "  {\"password\":\"%s\",\"info\":%s}%s\n",
                ctx->devices[i].password,
                info,
                (i + 1 == ctx->count) ? "" : ",");
    }
    fprintf(f, "]\n");
    fclose(f);
    return 0;
}

int storage_load_devices(struct DevicesContext *ctx, const char *path) {
    if (!ctx || !path) return -1;
    FILE *f = fopen(path, "r");
    if (!f) {
        return -1;
    }
    char *buffer = NULL;
    long len = 0;
    fseek(f, 0, SEEK_END);
    len = ftell(f);
    fseek(f, 0, SEEK_SET);
    buffer = (char *)malloc((size_t)len + 1);
    if (!buffer) {
        fclose(f);
        return -1;
    }
    fread(buffer, 1, (size_t)len, f);
    buffer[len] = '\0';
    fclose(f);

    memset(ctx, 0, sizeof(*ctx));
    const char *p = buffer;
    while ((p = strstr(p, "\"password\"")) && ctx->count < MAX_DEVICES) {
        struct Device *dev = &ctx->devices[ctx->count];
        memset(dev, 0, sizeof(*dev));
        json_get_string(p, "password", dev->password, sizeof(dev->password));
        const char *info_start = strstr(p, "\"info\"");
        if (!info_start) break;
        const char *brace = strchr(info_start, '{');
        if (!brace) break;
        int brace_count = 0;
        const char *q = brace;
        do {
            if (*q == '{') brace_count++;
            else if (*q == '}') brace_count--;
            q++;
        } while (brace_count > 0 && *q);
        size_t info_len = (size_t)(q - brace);
        char *info_json = (char *)malloc(info_len + 1);
        if (!info_json) break;
        memcpy(info_json, brace, info_len);
        info_json[info_len] = '\0';
        parse_device_info(dev, info_json);
        free(info_json);
        ctx->count++;
        p = q;
    }
    free(buffer);
    return ctx->count > 0 ? 0 : -1;
}
