#include "storage.h"
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

static const char *find_matching_brace(const char *start);

/** @brief Tìm vị trí chuỗi khóa `"key"` trong JSON. */
static const char *find_key(const char *json, const char *key) {
    if (!json || !key) return NULL;
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    return strstr(json, pattern);
}

/** @brief Lấy giá trị string cho `key`. */
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

/** @brief Lấy giá trị số thực cho `key`. */
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

/** @brief Lấy giá trị số nguyên cho `key`. */
static int json_get_int(const char *json, const char *key, int *out) {
    double v = 0.0;
    if (json_get_double(json, key, &v) != 0) return -1;
    *out = (int)v;
    return 0;
}

/** @brief Parse object JSON `info` và fill vào `Device` (theo type). */
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

/** @brief Escape chuỗi (chỉ xử lý `"` và `\\`) để ghi JSON. */
static void json_escape(const char *in, char *out, size_t out_len) {
    size_t pos = 0;
    for (size_t i = 0; in && in[i] && pos + 1 < out_len; ++i) {
        char c = in[i];
        if ((c == '"' || c == '\\') && pos + 2 < out_len) {
            out[pos++] = '\\';
            out[pos++] = c;
        } else {
            out[pos++] = c;
        }
    }
    out[pos] = '\0';
}

/** @see storage_save_farm() */
int storage_save_farm(const struct CoopsContext *coops, const struct DevicesContext *devices, const char *path) {
    if (!coops || !devices || !path) return -1;
    FILE *f = fopen(path, "w");
    if (!f) return -1;

    fprintf(f, "{\n  \"coops\": [\n");
    for (size_t i = 0; i < coops->count; ++i) {
        const struct CoopMeta *c = &coops->coops[i];
        char name_esc[MAX_COOP_NAME * 2];
        json_escape(c->name, name_esc, sizeof(name_esc));

        fprintf(f, "    {\"id\": %d, \"name\": \"%s\", \"devices\": [\n", c->id, name_esc);
        int first = 1;
        for (size_t j = 0; j < devices->count; ++j) {
            const struct Device *d = &devices->devices[j];
            if (d->identity.coop_id != c->id) continue;
            char info[MAX_JSON_LEN];
            devices_info_json(d, info, sizeof(info));
            fprintf(f, "      %s{\"password\":\"%s\",\"info\":%s}\n",
                    first ? "" : ",",
                    d->password,
                    info);
            first = 0;
        }
        fprintf(f, "    ]}%s\n", (i + 1 == coops->count) ? "" : ",");
    }
    fprintf(f, "  ]\n}\n");
    fclose(f);
    return 0;
}

/** @brief Tìm dấu `}` khớp với `{` bắt đầu tại `start` . */
static const char *find_matching_brace(const char *start) {
    int brace_count = 0;
    const char *p = start;
    if (!p || *p != '{') return NULL;
    do {
        if (*p == '{') brace_count++;
        else if (*p == '}') brace_count--;
        p++;
    } while (brace_count > 0 && *p);
    return (brace_count == 0) ? p : NULL;
}

int storage_load_farm(struct CoopsContext *coops, struct DevicesContext *devices, const char *path) {
    if (!coops || !devices || !path) return -1;
    FILE *f = fopen(path, "r");
    if (!f) return -1;

    fseek(f, 0, SEEK_END);
    long len = ftell(f);
    fseek(f, 0, SEEK_SET);
    char *buffer = (char *)malloc((size_t)len + 1);
    if (!buffer) {
        fclose(f);
        return -1;
    }
    fread(buffer, 1, (size_t)len, f);
    buffer[len] = '\0';
    fclose(f);

    coops_init(coops);
    memset(devices, 0, sizeof(*devices));

    const char *p = buffer;
    while (*p == ' ' || *p == '\n' || *p == '\r' || *p == '\t') p++;
    /* Chỉ chấp nhận format farm mới: object với key "coops"[] */
    if (*p == '[') {
        free(buffer);
        return -1;
    }
    const char *coops_key = strstr(buffer, "\"coops\"");
    if (!coops_key) {
        free(buffer);
        return -1;
    }
    const char *arr = strchr(coops_key, '[');
    if (!arr) {
        free(buffer);
        return -1;
    }
    const char *cur = arr;
    while ((cur = strchr(cur, '{')) && devices->count < MAX_DEVICES) {
        const char *end = find_matching_brace(cur);
        if (!end) break;
        size_t seg_len = (size_t)(end - cur);
        char *coop_json = (char *)malloc(seg_len + 1);
        if (!coop_json) break;
        memcpy(coop_json, cur, seg_len);
        coop_json[seg_len] = '\0';

        int coop_id = 0;
        char coop_name[MAX_COOP_NAME] = {0};
        if (json_get_int(coop_json, "id", &coop_id) == 0 &&
            json_get_string(coop_json, "name", coop_name, sizeof(coop_name)) == 0 &&
            coop_id > 0) {
            if (coop_name[0] == '\0' || strcmp(coop_name, "0") == 0) {
                snprintf(coop_name, sizeof(coop_name), "Chuong %d", coop_id);
            }
            (void)coops_upsert(coops, coop_id, coop_name);
            const char *dp = coop_json;
            while ((dp = strstr(dp, "\"password\"")) && devices->count < MAX_DEVICES) {
                struct Device *dev = &devices->devices[devices->count];
                memset(dev, 0, sizeof(*dev));
                json_get_string(dp, "password", dev->password, sizeof(dev->password));
                dev->identity.coop_id = coop_id;

                const char *info_start = strstr(dp, "\"info\"");
                if (!info_start) break;
                const char *brace = strchr(info_start, '{');
                if (!brace) break;
                const char *info_end = find_matching_brace(brace);
                if (!info_end) break;
                size_t info_len = (size_t)(info_end - brace);
                char *info_json = (char *)malloc(info_len + 1);
                if (!info_json) break;
                memcpy(info_json, brace, info_len);
                info_json[info_len] = '\0';
                parse_device_info(dev, info_json);
                free(info_json);
                devices->count++;
                dp = info_end;
            }
        }

        free(coop_json);
        cur = end;
        if (*cur == ']' || *cur == '\0') break;
    }

    if (coops->count == 0) {
        coops_init_default(coops);
    }

    for (size_t i = 0; i < devices->count; ++i) {
        if (devices->devices[i].identity.coop_id <= 0) {
            devices->devices[i].identity.coop_id = 1;
        }
        if (!coops_find(coops, devices->devices[i].identity.coop_id)) {
            char name[MAX_COOP_NAME];
            snprintf(name, sizeof(name), "Chuong %d", devices->devices[i].identity.coop_id);
            (void)coops_upsert(coops, devices->devices[i].identity.coop_id, name);
        }
    }

    free(buffer);
    return 0;
}
