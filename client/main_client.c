#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>

#include "ui_client.h"
#include "../server/devices.h"

#define MAX_SESSIONS 16

struct SessionEntry {
    char device_id[MAX_ID_LEN];
    char token[MAX_TOKEN_LEN];
};

struct DemoBackend {
    struct DevicesContext devices;
    struct SessionEntry sessions[MAX_SESSIONS];
    size_t session_count;
};

static struct SessionEntry *find_session(struct DemoBackend *b, const char *device_id, const char *token) {
    if (!b || !device_id || !token) return NULL;
    for (size_t i = 0; i < b->session_count; ++i) {
        if (strncmp(b->sessions[i].device_id, device_id, sizeof(b->sessions[i].device_id)) == 0 &&
            strncmp(b->sessions[i].token, token, sizeof(b->sessions[i].token)) == 0) {
            return &b->sessions[i];
        }
    }
    return NULL;
}

static struct SessionEntry *create_session(struct DemoBackend *b, const char *device_id, const char *token) {
    if (!b || !device_id || !token) return NULL;
    if (b->session_count >= MAX_SESSIONS) return NULL;
    struct SessionEntry *s = &b->sessions[b->session_count++];
    memset(s, 0, sizeof(*s));
    strncpy(s->device_id, device_id, sizeof(s->device_id) - 1);
    strncpy(s->token, token, sizeof(s->token) - 1);
    return s;
}

static void random_token(char *out, size_t len) {
    static const char alphabet[] = "0123456789abcdef";
    for (size_t i = 0; i + 1 < len; ++i) {
        out[i] = alphabet[rand() % (sizeof(alphabet) - 1)];
    }
    out[len - 1] = '\0';
}

static int backend_scan(void *user_data, struct DeviceIdentity *out, size_t max_out, size_t *found) {
    struct DemoBackend *b = (struct DemoBackend *)user_data;
    *found = devices_scan(&b->devices, out, max_out);
    return 0;
}

static int backend_connect(void *user_data, const char *device_id, const char *password, char *out_token, size_t token_len, enum DeviceType *out_type) {
    struct DemoBackend *b = (struct DemoBackend *)user_data;
    struct Device *dev = devices_find(&b->devices, device_id);
    if (!dev) {
        return -1;
    }
    if (strncmp(dev->password, password, sizeof(dev->password)) != 0) {
        return -2;
    }
    random_token(out_token, token_len);
    create_session(b, device_id, out_token);
    if (out_type) {
        *out_type = dev->identity.type;
    }
    return 0;
}

static int backend_require_token(struct DemoBackend *b, const char *device_id, const char *token) {
    return find_session(b, device_id, token) ? 0 : -1;
}

static int backend_info(void *user_data, const char *device_id, const char *token, char *out_json, size_t json_len) {
    struct DemoBackend *b = (struct DemoBackend *)user_data;
    if (backend_require_token(b, device_id, token) != 0) {
        return -1;
    }
    const struct Device *dev = devices_find_const(&b->devices, device_id);
    if (!dev) {
        return -2;
    }
    return devices_info_json(dev, out_json, json_len);
}

static int backend_control(void *user_data, const char *device_id, const char *token, const char *action, const char *payload) {
    struct DemoBackend *b = (struct DemoBackend *)user_data;
    if (backend_require_token(b, device_id, token) != 0) {
        return -1;
    }
    struct Device *dev = devices_find(&b->devices, device_id);
    if (!dev || !action) {
        return -2;
    }
    if (strcmp(action, "ON") == 0) {
        return devices_set_state(dev, DEVICE_ON);
    } else if (strcmp(action, "OFF") == 0) {
        return devices_set_state(dev, DEVICE_OFF);
    } else if (strcmp(action, "FEED_NOW") == 0 && dev->identity.type == DEVICE_FEEDER) {
        double food = dev->data.feeder.W;
        double water = dev->data.feeder.Vw;
        if (payload) {
            /* very small parser for {"food":X,"water":Y} */
            double f = 0.0, w = 0.0;
            if (sscanf(payload, "{\"food\":%lf,\"water\":%lf}", &f, &w) == 2) {
                food = f;
                water = w;
            }
        }
        return devices_feed_now(dev, food, water);
    } else if (strcmp(action, "DRINK_NOW") == 0 && dev->identity.type == DEVICE_DRINKER) {
        double water = dev->data.drinker.Vw;
        if (payload) {
            /* parser nho cho {"water":X} */
            double w = 0.0;
            if (sscanf(payload, "{\"water\":%lf}", &w) == 1) {
                water = w;
            }
        }
        return devices_drink_now(dev, water);
    } else if (strcmp(action, "SPRAY_NOW") == 0 && dev->identity.type == DEVICE_SPRAYER) {
        double Vh = dev->data.sprayer.Vh;
        if (payload) {
            /* parser nho cho {"Vh":X} */
            double v = 0.0;
            if (sscanf(payload, "{\"Vh\":%lf}", &v) == 1) {
                Vh = v;
            }
        }
        return devices_spray_now(dev, Vh);
    }
    return -3;
}

static int backend_setcfg(void *user_data, const char *device_id, const char *token, const char *json_payload) {
    struct DemoBackend *b = (struct DemoBackend *)user_data;
    if (backend_require_token(b, device_id, token) != 0) {
        return -1;
    }
    struct Device *dev = devices_find(&b->devices, device_id);
    if (!dev || !json_payload) {
        return -2;
    }
    switch (dev->identity.type) {
    case DEVICE_FAN: {
        double Tmax = dev->data.fan.Tmax, Tp1 = dev->data.fan.Tp1;
        sscanf(json_payload, "{\"Tmax\":%lf,\"Tp1\":%lf}", &Tmax, &Tp1);
        return devices_set_config_fan(dev, Tmax, Tp1);
    }
    case DEVICE_HEATER: {
        double Tmin = dev->data.heater.Tmin, Tp2 = dev->data.heater.Tp2;
        sscanf(json_payload, "{\"Tmin\":%lf,\"Tp2\":%lf}", &Tmin, &Tp2);
        return devices_set_config_heater(dev, Tmin, Tp2, dev->data.heater.mode);
    }
    case DEVICE_SPRAYER: {
        double Hmin = dev->data.sprayer.Hmin, Hp = dev->data.sprayer.Hp, Vh = dev->data.sprayer.Vh;
        sscanf(json_payload, "{\"Hmin\":%lf,\"Hp\":%lf,\"Vh\":%lf}", &Hmin, &Hp, &Vh);
        return devices_set_config_sprayer(dev, Hmin, Hp, Vh);
    }
    case DEVICE_FEEDER: {
        double W = dev->data.feeder.W, Vw = dev->data.feeder.Vw;
        sscanf(json_payload, "{\"W\":%lf,\"Vw\":%lf}", &W, &Vw);
        return devices_set_config_feeder(dev, W, Vw, dev->data.feeder.schedule, dev->data.feeder.schedule_count);
    }
    case DEVICE_DRINKER: {
        double Vw = dev->data.drinker.Vw;
        sscanf(json_payload, "{\"Vw\":%lf}", &Vw);
        return devices_set_config_drinker(dev, Vw, dev->data.drinker.schedule, dev->data.drinker.schedule_count);
    }
    default:
        return -3;
    }
}

int main(void) {
    struct DemoBackend backend;
    devices_context_init(&backend.devices);
    backend.session_count = 0;
    srand((unsigned int)time(NULL));

    struct UiBackendOps ops = {
        .user_data = &backend,
        .scan = backend_scan,
        .connect = backend_connect,
        .info = backend_info,
        .control = backend_control,
        .setcfg = backend_setcfg
    };

    struct UiContext ui;
    ui_context_init(&ui, &ops);
    ui_run(&ui);
    return 0;
}
