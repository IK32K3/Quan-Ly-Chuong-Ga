#include "../server/coop_logic.h"
#include "devices.h"
#include "session_auth.h"
#include "monitor_log.h"
#include "net_server.h"
#include "storage.h"
#include "../shared/protocol.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Biến dùng chung cho B */
static struct DevicesContext g_devices;

void coop_logic_init(void) {
    /* Thu tai tu file, neu khong co thi khoi tao mac dinh */
    if (storage_load_devices(&g_devices, "devices_state.json") != 0) {
        devices_context_init(&g_devices);
    }
}

/* Tiện ích cấp phát response */
static char *alloc_line(const char *src) {
    size_t len = strlen(src) + 1;
    char *buf = (char *)malloc(len);
    if (buf) {
        memcpy(buf, src, len);
    }
    return buf;
}

/* Parse số thực đơn giản */
static void parse_two_doubles(const char *payload, const char *fmt, double *a, double *b) {
    if (payload && fmt && a && b) {
        sscanf(payload, fmt, a, b);
    }
}

/* Gửi nhiều dòng cho SCAN */
static void handle_scan(int fd) {
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
        protocol_format_device(line, sizeof(line), list[i].id, list[i].type);
        send_line(fd, line);
    }
}

char *handle_command(int fd, enum CommandType cmd, char *args) {
    /* Lưu ý: net_server sẽ gửi response trả về; riêng SCAN tự gửi nhiều dòng và trả NULL */
    char line[MAX_LINE_LEN];
    switch (cmd) {
    case CMD_SCAN:
        handle_scan(fd);
        return NULL;
    case CMD_CONNECT: {
        char dev_id[MAX_ID_LEN], app_id[32], password[MAX_PASSWORD_LEN];
        if (!args || sscanf(args, "%31s %31s %63s", dev_id, app_id, password) != 3) {
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
        const struct Device *dev = devices_find_const(&g_devices, dev_id);
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
            double food = dev->data.feeder.W, water = dev->data.feeder.Vw;
            parse_two_doubles(rest, "{\"food\":%lf,\"water\":%lf}", &food, &water);
            rc = devices_feed_now(dev, food, water);
        }
        if (rc != 0) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        log_device_event(dev_id, action);
        storage_save_devices(&g_devices, "devices_state.json");
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
            double Tmax = dev->data.fan.Tmax, Tp1 = dev->data.fan.Tp1;
            parse_two_doubles(json_payload, "{\"Tmax\":%lf,\"Tp1\":%lf}", &Tmax, &Tp1);
            rc = devices_set_config_fan(dev, Tmax, Tp1);
        } else if (dev->identity.type == DEVICE_HEATER) {
            double Tmin = dev->data.heater.Tmin, Tp2 = dev->data.heater.Tp2;
            parse_two_doubles(json_payload, "{\"Tmin\":%lf,\"Tp2\":%lf}", &Tmin, &Tp2);
            rc = devices_set_config_heater(dev, Tmin, Tp2, dev->data.heater.mode);
        } else if (dev->identity.type == DEVICE_SPRAYER) {
            double Hmin = dev->data.sprayer.Hmin, Hp = dev->data.sprayer.Hp, Vh = dev->data.sprayer.Vh;
            sscanf(json_payload, "{\"Hmin\":%lf,\"Hp\":%lf,\"Vh\":%lf}", &Hmin, &Hp, &Vh);
            rc = devices_set_config_sprayer(dev, Hmin, Hp, Vh);
        } else if (dev->identity.type == DEVICE_FEEDER) {
            double W = dev->data.feeder.W, Vw = dev->data.feeder.Vw;
            parse_two_doubles(json_payload, "{\"W\":%lf,\"Vw\":%lf}", &W, &Vw);
            rc = devices_set_config_feeder(dev, W, Vw, dev->data.feeder.schedule, dev->data.feeder.schedule_count);
        } else if (dev->identity.type == DEVICE_DRINKER) {
            double Vw = dev->data.drinker.Vw;
            sscanf(json_payload, "{\"Vw\":%lf}", &Vw);
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
        storage_save_devices(&g_devices, "devices_state.json");
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
        storage_save_devices(&g_devices, "devices_state.json");
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
        if (!args || sscanf(args, "%31s %15s %63s", dev_id, type_str, pw) != 3) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        enum DeviceType type = device_type_from_string(type_str);
        if (type == DEVICE_UNKNOWN) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        if (devices_add(&g_devices, dev_id, type, pw) != 0) {
            protocol_format_bad_request(line, sizeof(line));
            return alloc_line(line);
        }
        storage_save_devices(&g_devices, "devices_state.json");
        log_device_event(dev_id, "ADD_DEVICE");
        protocol_format_add_ok(line, sizeof(line));
        return alloc_line(line);
    }
    default:
        protocol_format_bad_request(line, sizeof(line));
        return alloc_line(line);
    }
}
