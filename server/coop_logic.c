#include "../server/coop_logic.h"
#include "coops.h"
#include "devices.h"
#include "session_auth.h"
#include "monitor_log.h"
#include "net_server.h"
#include "storage.h"
#include "../shared/protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <jansson.h>

static struct CoopsContext g_coops;
static struct DevicesContext g_devices;

static const char *FARM_STATE_PATH = "farm_state.json";

/** @brief Nạp thêm dữ liệu farm từ đĩa và merge vào state hiện tại. */
static void merge_farm_from_disk(void) {
    struct CoopsContext file_coops;
    struct DevicesContext file_devices;
    if (storage_load_farm(&file_coops, &file_devices, FARM_STATE_PATH) != 0) {
        return;
    }

    for (size_t i = 0; i < file_coops.count; ++i) {
        (void)coops_upsert(&g_coops, file_coops.coops[i].id, file_coops.coops[i].name);
    }

    for (size_t i = 0; i < file_devices.count; ++i) {
        if (g_devices.count >= MAX_DEVICES) break;
        const struct Device *d = &file_devices.devices[i];
        if (devices_find(&g_devices, d->identity.id)) continue;
        g_devices.devices[g_devices.count++] = *d;
    }
}

/** @brief Chuẩn hoá tên chuồng: nếu rỗng/"0" thì gán mặc định "Chuong <id>". */
static void sanitize_coop_names(void) {
    for (size_t i = 0; i < g_coops.count; ++i) {
        if (g_coops.coops[i].name[0] == '\0' || strcmp(g_coops.coops[i].name, "0") == 0) {
            snprintf(g_coops.coops[i].name, sizeof(g_coops.coops[i].name), "Chuong %d", g_coops.coops[i].id);
        }
    }
}

void coop_logic_init(void) {
    /* Thu tai tu file farm_state.json, neu khong co thi khoi tao mac dinh */
    if (storage_load_farm(&g_coops, &g_devices, FARM_STATE_PATH) != 0) {
        coops_init(&g_coops);
        devices_context_init(&g_devices);
    }
    sanitize_coop_names();
}

/* Tiện ích cấp phát response */
/** @brief Cấp phát và copy một dòng response (caller phải `free()`). */
static char *alloc_line(const char *src) {
    size_t len = strlen(src) + 1;
    char *buf = (char *)malloc(len);
    if (buf) {
        memcpy(buf, src, len);
    }
    return buf;
}

static json_t *parse_payload_object(const char *payload) {
    if (!payload) return NULL;
    while (*payload == ' ') payload++;
    if (*payload != '{') return NULL;
    json_t *root = json_loads(payload, 0, NULL);
    if (!json_is_object(root)) {
        if (root) json_decref(root);
        return NULL;
    }
    return root;
}

static int require_number(json_t *obj, const char *key, double *out) {
    if (!obj || !key || !out) return -1;
    json_t *v = json_object_get(obj, key);
    if (!json_is_number(v)) return -1;
    *out = json_number_value(v);
    return 0;
}

static int require_int(json_t *obj, const char *key, int *out) {
    if (!obj || !key || !out) return -1;
    json_t *v = json_object_get(obj, key);
    if (json_is_integer(v)) {
        *out = (int)json_integer_value(v);
        return 0;
    }
    if (json_is_number(v)) {
        *out = (int)json_number_value(v);
        return 0;
    }
    return -1;
}

/* Gửi nhiều dòng cho SCAN */
/** @brief Xử lý command SCAN: gửi 0..N dòng RESP_DEVICE trực tiếp về client. */
static void handle_scan(int fd) {
    /* Neu user edit farm_state.json ben ngoai, SCAN se nap them cac thiet bi moi */
    merge_farm_from_disk();

    struct DeviceIdentity list[MAX_DEVICES];
    size_t found = devices_scan(&g_devices, list, MAX_DEVICES);
    if (found == 0) {
        char line[MAX_LINE_LEN];
        protocol_format_no_device_scan(line, sizeof(line));
        send_line(fd, line);
        return;
    }
    for (size_t i = 0; i < found; ++i) {
        char line[MAX_LINE_LEN];
        protocol_format_device_ex(line, sizeof(line), list[i].id, list[i].type, list[i].coop_id);
        send_line(fd, line);
    }
}

/** @brief Xử lý command COOPLIST: gửi 0..N dòng RESP_COOP trực tiếp về client. */
static void handle_coop_list(int fd) {
    if (g_coops.count == 0) {
        char line[MAX_LINE_LEN];
        protocol_format_no_coop(line, sizeof(line));
        send_line(fd, line);
        return;
    }
    for (size_t i = 0; i < g_coops.count; ++i) {
        char line[MAX_LINE_LEN];
        protocol_format_coop(line, sizeof(line), g_coops.coops[i].id, g_coops.coops[i].name);
        send_line(fd, line);
    }
}

/**
 * @brief Router xử lý command .
 */
char *handle_command(int fd, enum CommandType cmd, char *args) {
    char line[MAX_LINE_LEN];
    switch (cmd) {
    case CMD_SCAN:
        handle_scan(fd);
        return NULL;
    case CMD_COOP_LIST:
        handle_coop_list(fd);
        return NULL;
    case CMD_COOP_ADD: {
        if (!args || args[0] == '\0') {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        int new_id = 0;
        if (coops_add(&g_coops, args, &new_id) != 0) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        (void)storage_save_farm(&g_coops, &g_devices, FARM_STATE_PATH);
        protocol_format_coopadd_ok(line, sizeof(line), new_id);
        return alloc_line(line);
    }
    case CMD_CONNECT: {
        char dev_id[MAX_ID_LEN], password[MAX_PASSWORD_LEN];
        /* Format: CONNECT <device_id> <app_id> <password> (app_id hiện chưa dùng). */
        if (!args || sscanf(args, "%31s %*31s %63s", dev_id, password) != 2) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        struct Device *dev = devices_find(&g_devices, dev_id);
        if (!dev) {
            protocol_format_no_device_err(line, sizeof(line));
            return alloc_line(line);
        }
        if (strncmp(dev->password, password, sizeof(dev->password)) != 0) {
            protocol_format_wrong_password(line, sizeof(line));
            return alloc_line(line);
        }
        char token[MAX_TOKEN_LEN];
        if (create_session(dev_id, token) != 0) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        log_device_event(dev_id, "CONNECT_OK");
        protocol_format_connect_ok(line, sizeof(line), token);
        return alloc_line(line);
    }
    case CMD_INFO: {
        char dev_id[MAX_ID_LEN], token[MAX_TOKEN_LEN];
        if (!args || sscanf(args, "%31s %63s", dev_id, token) != 2) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        char validated[MAX_ID_LEN];
        if (validate_session(token, validated) != 0 || strncmp(validated, dev_id, sizeof(validated)) != 0) {
            protocol_format_not_connected(line, sizeof(line));
            return alloc_line(line);
        }
        const struct Device *dev = devices_find(&g_devices, dev_id);
        if (!dev) {
            protocol_format_no_device_err(line, sizeof(line));
            return alloc_line(line);
        }
        char json[MAX_JSON_LEN];
        if (devices_info_json(dev, json, sizeof(json)) != 0) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        protocol_format_info_ok(line, sizeof(line), json);
        return alloc_line(line);
    }
    case CMD_CONTROL: {
        char dev_id[MAX_ID_LEN], token[MAX_TOKEN_LEN], action[MAX_ACTION_LEN];
        /* payload (nếu có) đặt sau action */
        if (!args) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        /* Tách action */
        char *rest = NULL;
        if (sscanf(args, "%31s %63s %15s", dev_id, token, action) < 3) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        rest = strstr(args, action);
        if (rest) {
            rest += strlen(action);
            while (*rest == ' ') rest++;
        }
        char validated[MAX_ID_LEN];
        if (validate_session(token, validated) != 0 || strncmp(validated, dev_id, sizeof(validated)) != 0) {
            protocol_format_not_connected(line, sizeof(line));
            return alloc_line(line);
        }
        struct Device *dev = devices_find(&g_devices, dev_id);
        if (!dev) {
            protocol_format_no_device_err(line, sizeof(line));
            return alloc_line(line);
        }
        int rc = -1;
        if (strcmp(action, "ON") == 0) {
            rc = devices_set_state(dev, DEVICE_ON);
        } else if (strcmp(action, "OFF") == 0) {
            rc = devices_set_state(dev, DEVICE_OFF);
        } else if (strcmp(action, "FEED_NOW") == 0 && dev->identity.type == DEVICE_FEEDER) {
            json_t *payload = parse_payload_object(rest);
            double food = 0.0, water = 0.0;
            if (!payload ||
                require_number(payload, "thuc_an_kg", &food) != 0 ||
                require_number(payload, "nuoc_l", &water) != 0) {
                if (payload) json_decref(payload);
                protocol_format_bad_request(line, sizeof(line));
                return alloc_line(line);
            }
            json_decref(payload);
            rc = devices_feed_now(dev, food, water);
        } else if (strcmp(action, "DRINK_NOW") == 0 && dev->identity.type == DEVICE_DRINKER) {
            json_t *payload = parse_payload_object(rest);
            double water = 0.0;
            if (!payload || require_number(payload, "nuoc_l", &water) != 0) {
                if (payload) json_decref(payload);
                protocol_format_bad_request(line, sizeof(line));
                return alloc_line(line);
            }
            json_decref(payload);
            rc = devices_drink_now(dev, water);
        } else if (strcmp(action, "SPRAY_NOW") == 0 && dev->identity.type == DEVICE_SPRAYER) {
            json_t *payload = parse_payload_object(rest);
            double Vh = 0.0;
            if (!payload || require_number(payload, "luu_luong_lph", &Vh) != 0) {
                if (payload) json_decref(payload);
                protocol_format_bad_request(line, sizeof(line));
                return alloc_line(line);
            }
            json_decref(payload);
            rc = devices_spray_now(dev, Vh);
        }
        if (rc != 0) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        log_device_event(dev_id, action);
        (void)storage_save_farm(&g_coops, &g_devices, FARM_STATE_PATH);
        protocol_format_control_ok(line, sizeof(line));
        return alloc_line(line);
    }
    case CMD_SETCFG: {
        char dev_id[MAX_ID_LEN], token[MAX_TOKEN_LEN];
        if (!args) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        if (sscanf(args, "%31s %63s", dev_id, token) < 2) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        char validated[MAX_ID_LEN];
        if (validate_session(token, validated) != 0 || strncmp(validated, dev_id, sizeof(validated)) != 0) {
            protocol_format_not_connected(line, sizeof(line));
            return alloc_line(line);
        }
        char *json_payload = strstr(args, token);
        if (json_payload) {
            json_payload += strlen(token);
            while (*json_payload == ' ') json_payload++;
        }
        struct Device *dev = devices_find(&g_devices, dev_id);
        if (!dev) {
            protocol_format_no_device_err(line, sizeof(line));
            return alloc_line(line);
        }
        int rc = -1;
        if (dev->identity.type == DEVICE_FAN) {
            json_t *payload = parse_payload_object(json_payload);
            int speed = 0;
            if (!payload ||
                require_int(payload, "toc_do", &speed) != 0) {
                if (payload) json_decref(payload);
                protocol_format_bad_request(line, sizeof(line));
                return alloc_line(line);
            }
            json_decref(payload);
            rc = devices_set_config_fan(dev, speed);
        } else if (dev->identity.type == DEVICE_HEATER) {
            json_t *payload = parse_payload_object(json_payload);
            double Tmin = 0.0, Tp2 = 0.0;
            if (!payload ||
                require_number(payload, "nhiet_do_bat_c", &Tmin) != 0 ||
                require_number(payload, "nhiet_do_tat_c", &Tp2) != 0) {
                if (payload) json_decref(payload);
                protocol_format_bad_request(line, sizeof(line));
                return alloc_line(line);
            }
            json_decref(payload);
            rc = devices_set_config_heater(dev, Tmin, Tp2, dev->data.heater.mode);
        } else if (dev->identity.type == DEVICE_SPRAYER) {
            json_t *payload = parse_payload_object(json_payload);
            double Hmin = 0.0, Hp = 0.0, Vh = 0.0;
            if (!payload ||
                require_number(payload, "do_am_bat_pct", &Hmin) != 0 ||
                require_number(payload, "do_am_muc_tieu_pct", &Hp) != 0 ||
                require_number(payload, "luu_luong_lph", &Vh) != 0) {
                if (payload) json_decref(payload);
                protocol_format_bad_request(line, sizeof(line));
                return alloc_line(line);
            }
            json_decref(payload);
            rc = devices_set_config_sprayer(dev, Hmin, Hp, Vh);
        } else if (dev->identity.type == DEVICE_FEEDER) {
            json_t *payload = parse_payload_object(json_payload);
            double W = 0.0, Vw = 0.0;
            if (!payload ||
                require_number(payload, "thuc_an_kg", &W) != 0 ||
                require_number(payload, "nuoc_l", &Vw) != 0) {
                if (payload) json_decref(payload);
                protocol_format_bad_request(line, sizeof(line));
                return alloc_line(line);
            }
            json_decref(payload);
            rc = devices_set_config_feeder(dev, W, Vw, dev->data.feeder.schedule, dev->data.feeder.schedule_count);
        } else if (dev->identity.type == DEVICE_DRINKER) {
            json_t *payload = parse_payload_object(json_payload);
            double Vw = 0.0;
            if (!payload || require_number(payload, "nuoc_l", &Vw) != 0) {
                if (payload) json_decref(payload);
                protocol_format_bad_request(line, sizeof(line));
                return alloc_line(line);
            }
            json_decref(payload);
            rc = devices_set_config_drinker(dev, Vw, dev->data.drinker.schedule, dev->data.drinker.schedule_count);
        }
        if (rc != 0) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        char json[MAX_JSON_LEN];
        devices_info_json(dev, json, sizeof(json));
        protocol_format_setcfg_ok(line, sizeof(line), json);
        log_device_event(dev_id, "SETCFG");
        (void)storage_save_farm(&g_coops, &g_devices, FARM_STATE_PATH);
        return alloc_line(line);
    }
    case CMD_CHPASS: {
        char dev_id[MAX_ID_LEN], token[MAX_TOKEN_LEN], old_pw[MAX_PASSWORD_LEN], new_pw[MAX_PASSWORD_LEN];
        if (!args || sscanf(args, "%31s %63s %63s %63s", dev_id, token, old_pw, new_pw) != 4) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        char validated[MAX_ID_LEN];
        if (validate_session(token, validated) != 0 || strncmp(validated, dev_id, sizeof(validated)) != 0) {
            protocol_format_not_connected(line, sizeof(line));
            return alloc_line(line);
        }
        struct Device *dev = devices_find(&g_devices, dev_id);
        if (!dev) {
            protocol_format_no_device_err(line, sizeof(line));
            return alloc_line(line);
        }
        if (devices_change_password(dev, old_pw, new_pw) != 0) {
            protocol_format_wrong_password(line, sizeof(line));
            return alloc_line(line);
        }
        log_device_event(dev_id, "CHPASS");
        (void)storage_save_farm(&g_coops, &g_devices, FARM_STATE_PATH);
        protocol_format_pass_ok(line, sizeof(line));
        return alloc_line(line);
    }
    case CMD_BYE: {
        char dev_id[MAX_ID_LEN], token[MAX_TOKEN_LEN];
        if (!args || sscanf(args, "%31s %63s", dev_id, token) != 2) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        char validated[MAX_ID_LEN];
        if (validate_session(token, validated) != 0 || strncmp(validated, dev_id, sizeof(validated)) != 0) {
            protocol_format_not_connected(line, sizeof(line));
            return alloc_line(line);
        }
        end_session(token);
        log_device_event(dev_id, "BYE");
        protocol_format_bye_ok(line, sizeof(line));
        return alloc_line(line);
    }
    case CMD_ADD_DEVICE: {
        char dev_id[MAX_ID_LEN], type_str[MAX_TYPE_LEN], pw[MAX_PASSWORD_LEN];
        int coop_id = 0;
        int parsed = args ? sscanf(args, "%31s %15s %63s %d", dev_id, type_str, pw, &coop_id) : 0;
        if (parsed < 3) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        enum DeviceType type = device_type_from_string(type_str);
        if (type == DEVICE_UNKNOWN) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        if (coop_id > 0 && !coops_find(&g_coops, coop_id)) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        if (coop_id <= 0) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        if (devices_add(&g_devices, dev_id, type, pw, coop_id) != 0) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        (void)storage_save_farm(&g_coops, &g_devices, FARM_STATE_PATH);
        log_device_event(dev_id, "ADD_DEVICE");
        protocol_format_add_ok(line, sizeof(line));
        return alloc_line(line);
    }
    case CMD_ASSIGN_DEVICE: {
        char dev_id[MAX_ID_LEN];
        int coop_id = 0;
        if (!args || sscanf(args, "%31s %d", dev_id, &coop_id) != 2) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        if (coop_id <= 0) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        if (!coops_find(&g_coops, coop_id)) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        struct Device *dev = devices_find(&g_devices, dev_id);
        if (!dev) {
            protocol_format_no_device_err(line, sizeof(line));
            return alloc_line(line);
        }
        dev->identity.coop_id = coop_id;
        (void)storage_save_farm(&g_coops, &g_devices, FARM_STATE_PATH);
        log_device_event(dev_id, "ASSIGN_DEVICE");
        protocol_format_assign_ok(line, sizeof(line));
        return alloc_line(line);
    }
    default:
        protocol_format_bad_request(line, sizeof(line));
        return alloc_line(line);
    }
}
