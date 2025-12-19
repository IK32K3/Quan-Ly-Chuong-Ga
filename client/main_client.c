#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "ui_client.h"
#include "net_client.h"
#include "../shared/protocol.h"

struct NetBackend {
    int fd;
};

/**
 * @file main_client.c
 * @brief Entry point client: hiện menu UI và giao tiếp server qua TCP.
 */

/** @brief Parse 1 dòng response theo format: "<code> <text> [payload...]". */
static int parse_response(const char *line, int *code, char *text, size_t text_len, char *payload, size_t payload_len) {
    (void)payload_len;
    if (!line || !code || !text || text_len == 0) return -1;
    payload[0] = '\0';
    int c = 0;
    int count = sscanf(line, "%d %63s %[^\n]", &c, text, payload);
    if (count < 2) {
        return -1;
    }
    *code = c;
    return 0;
}

/** @brief Backend UI: SCAN (đọc nhiều dòng RESP_DEVICE cho tới khi timeout). */
static int backend_scan(void *user_data, struct DeviceIdentity *out, size_t max_out, size_t *found) {
    struct NetBackend *b = (struct NetBackend *)user_data;
    *found = 0;
    if (client_send_line(b->fd, "SCAN") != 0) return -1;

    char buf[MAX_LINE_LEN];
    while (1) {
        int r = client_recv_line_timeout(b->fd, buf, sizeof(buf), 200);
        if (r == 1) { /* het du lieu */
            break;
        } else if (r < 0) {
            return -1;
        }
        int code = 0;
        char text[64], payload[MAX_LINE_LEN] = {0};
        if (parse_response(buf, &code, text, sizeof(text), payload, sizeof(payload)) != 0) {
            continue;
        }
        if (code == RESP_DEVICE) {
            char id[MAX_ID_LEN], type_str[MAX_TYPE_LEN];
            int coop_id = 0;
            if (strcmp(text, "DEVICE") != 0) {
                continue;
            }
            int n = sscanf(payload, "%31s %15s %d", id, type_str, &coop_id);
            if (n >= 2) {
                if (*found < max_out) {
                    strncpy(out[*found].id, id, sizeof(out[*found].id) - 1);
                    out[*found].id[sizeof(out[*found].id) - 1] = '\0';
                    out[*found].type = device_type_from_string(type_str);
                    out[*found].coop_id = (n == 3) ? coop_id : 0;
                    (*found)++;
                }
            }
        } else if (code == RESP_NO_DEVICE_SCAN) {
            *found = 0;
            break;
        } else {
            /* response khac => ket thuc vong */
            break;
        }
    }
    return 0;
}

/** @brief Backend UI: CONNECT (lấy token). */
static int backend_connect(void *user_data, const char *device_id, const char *password, char *out_token, size_t token_len, enum DeviceType *out_type) {
    struct NetBackend *b = (struct NetBackend *)user_data;
    char line[MAX_LINE_LEN];
    snprintf(line, sizeof(line), "CONNECT %s APP1 %s", device_id, password);
    if (client_send_line(b->fd, line) != 0) return -1;
    if (client_recv_line(b->fd, line, sizeof(line)) != 0) return -1;
    int code = 0; char text[64]; char payload[MAX_LINE_LEN] = {0};
    if (parse_response(line, &code, text, sizeof(text), payload, sizeof(payload)) != 0) return -1;
    if (code != RESP_CONNECT_OK) return -1;
    strncpy(out_token, payload, token_len - 1);
    out_token[token_len - 1] = '\0';
    (void)out_type; /* type da duoc biet tu SCAN/UI */
    return 0;
}

/** @brief Backend UI: INFO (lấy JSON). */
static int backend_info(void *user_data, const char *device_id, const char *token, char *out_json, size_t json_len) {
    struct NetBackend *b = (struct NetBackend *)user_data;
    char line[MAX_LINE_LEN];
    snprintf(line, sizeof(line), "INFO %s %s", device_id, token);
    if (client_send_line(b->fd, line) != 0) return -1;
    if (client_recv_line(b->fd, line, sizeof(line)) != 0) return -1;
    int code = 0; char text[64]; char payload[MAX_JSON_LEN] = {0};
    if (parse_response(line, &code, text, sizeof(text), payload, sizeof(payload)) != 0) return -1;
    if (code != RESP_INFO_OK) return -1;
    strncpy(out_json, payload, json_len - 1);
    out_json[json_len - 1] = '\0';
    return 0;
}

/** @brief Backend UI: CONTROL (ON/OFF/FEED_NOW/...). */
static int backend_control(void *user_data, const char *device_id, const char *token, const char *action, const char *payload) {
    struct NetBackend *b = (struct NetBackend *)user_data;
    char line[MAX_LINE_LEN];
    if (payload && payload[0]) {
        snprintf(line, sizeof(line), "CONTROL %s %s %s %s", device_id, token, action, payload);
    } else {
        snprintf(line, sizeof(line), "CONTROL %s %s %s", device_id, token, action);
    }
    if (client_send_line(b->fd, line) != 0) return -1;
    if (client_recv_line(b->fd, line, sizeof(line)) != 0) return -1;
    int code = 0; char text[64]; char resp_payload[64] = {0};
    if (parse_response(line, &code, text, sizeof(text), resp_payload, sizeof(resp_payload)) != 0) return -1;
    return code == RESP_CONTROL_OK ? 0 : -1;
}

/** @brief Backend UI: SETCFG (gửi JSON config). */
static int backend_setcfg(void *user_data, const char *device_id, const char *token, const char *json_payload) {
    struct NetBackend *b = (struct NetBackend *)user_data;
    char line[MAX_LINE_LEN];
    snprintf(line, sizeof(line), "SETCFG %s %s %s", device_id, token, json_payload);
    if (client_send_line(b->fd, line) != 0) return -1;
    if (client_recv_line(b->fd, line, sizeof(line)) != 0) return -1;
    int code = 0; char text[64]; char resp_payload[MAX_JSON_LEN] = {0};
    if (parse_response(line, &code, text, sizeof(text), resp_payload, sizeof(resp_payload)) != 0) return -1;
    return code == RESP_SETCFG_OK ? 0 : -1;
}

/** @brief Backend UI: ADD device mới (đăng ký vào coop). */
static int backend_add_device(void *user_data, const char *device_id, enum DeviceType type, const char *password, int coop_id) {
    struct NetBackend *b = (struct NetBackend *)user_data;
    if (!device_id || device_id[0] == '\0') return -1;
    if (coop_id <= 0) return -1;
    if (!password || password[0] == '\0') password = "123456";

    char line[MAX_LINE_LEN];
    snprintf(line, sizeof(line), "ADD %s %s %s %d", device_id, device_type_to_string(type), password, coop_id);
    if (client_send_line(b->fd, line) != 0) return -1;
    if (client_recv_line(b->fd, line, sizeof(line)) != 0) return -1;
    int code = 0; char text[64]; char payload[64] = {0};
    if (parse_response(line, &code, text, sizeof(text), payload, sizeof(payload)) != 0) return -1;
    return code == RESP_ADD_OK ? 0 : -1;
}

/** @brief Backend UI: ASSIGN thiết bị sang coop khác. */
static int backend_assign(void *user_data, const char *device_id, int coop_id) {
    struct NetBackend *b = (struct NetBackend *)user_data;
    char line[MAX_LINE_LEN];
    snprintf(line, sizeof(line), "ASSIGN %s %d", device_id, coop_id);
    if (client_send_line(b->fd, line) != 0) return -1;
    if (client_recv_line(b->fd, line, sizeof(line)) != 0) return -1;
    int code = 0; char text[64]; char payload[64] = {0};
    if (parse_response(line, &code, text, sizeof(text), payload, sizeof(payload)) != 0) return -1;
    return code == RESP_ASSIGN_OK ? 0 : -1;
}

/** @brief Backend UI: COOPLIST (đọc nhiều dòng RESP_COOP cho tới khi timeout). */
static int backend_coop_list(void *user_data, struct CoopList *out) {
    struct NetBackend *b = (struct NetBackend *)user_data;
    if (!out) return -1;
    coop_list_init(out);
    if (client_send_line(b->fd, "COOPLIST") != 0) return -1;

    char buf[MAX_LINE_LEN];
    while (1) {
        int r = client_recv_line_timeout(b->fd, buf, sizeof(buf), 200);
        if (r == 1) break;
        if (r < 0) return -1;
        int code = 0;
        char text[64], payload[MAX_LINE_LEN] = {0};
        if (parse_response(buf, &code, text, sizeof(text), payload, sizeof(payload)) != 0) continue;
        if (code == RESP_NO_COOP) {
            break;
        }
        if (code != RESP_COOP) {
            break;
        }
        if (strcmp(text, "COOP") != 0) {
            continue;
        }
        int id = 0;
        char name[MAX_COOP_NAME] = {0};
        if (sscanf(payload, "%d %63[^\n]", &id, name) == 2) {
            if (out->count < MAX_COOPS) {
                if (strcmp(name, "0") == 0) {
                    continue;
                }
                struct Coop *c = &out->coops[out->count++];
                memset(c, 0, sizeof(*c));
                c->id = id;
                strncpy(c->name, name, sizeof(c->name) - 1);
                c->name[sizeof(c->name) - 1] = '\0';
            }
        }
    }
    return 0;
}

/** @brief Backend UI: COOPADD (thêm chuồng mới). */
static int backend_coop_add(void *user_data, const char *name, int *out_id) {
    struct NetBackend *b = (struct NetBackend *)user_data;
    if (!name || name[0] == '\0') return -1;
    char line[MAX_LINE_LEN];
    snprintf(line, sizeof(line), "COOPADD %s", name);
    if (client_send_line(b->fd, line) != 0) return -1;
    if (client_recv_line(b->fd, line, sizeof(line)) != 0) return -1;
    int code = 0;
    char text[64], payload[MAX_LINE_LEN] = {0};
    if (parse_response(line, &code, text, sizeof(text), payload, sizeof(payload)) != 0) return -1;
    if (code != RESP_COOPADD_OK) return -1;
    int id = atoi(payload);
    if (out_id) *out_id = id;
    return 0;
}

/** @brief Đọc input từ stdin với prompt và hỗ trợ default nếu người dùng bỏ trống. */
static void read_input_line(const char *prompt, char *out, size_t out_len, const char *default_val) {
    printf("%s", prompt);
    if (fgets(out, (int)out_len, stdin)) {
        size_t l = strlen(out);
        if (l > 0 && out[l - 1] == '\n') out[l - 1] = '\0';
        if (out[0] == '\0' && default_val) {
            strncpy(out, default_val, out_len - 1);
            out[out_len - 1] = '\0';
        }
    } else if (default_val) {
        strncpy(out, default_val, out_len - 1);
        out[out_len - 1] = '\0';
    }
}

/** @brief Entry point client: parse ip/port, kết nối server và chạy UI. */
int main(int argc, char **argv) {
    /* Ho tro: ./client_app <ip> <port> */
    char host[64] = "127.0.0.1";
    int port = 8888;

    if (argc >= 2) {
        strncpy(host, argv[1], sizeof(host) - 1);
        host[sizeof(host) - 1] = '\0';
    } else {
        read_input_line("Nhap IP server (mac dinh 127.0.0.1): ", host, sizeof(host), "127.0.0.1");
    }
    if (argc >= 3) {
        port = atoi(argv[2]);
    } else {
        char port_str[16];
        read_input_line("Nhap cong (mac dinh 8888): ", port_str, sizeof(port_str), "8888");
        port = atoi(port_str);
    }
    if (port <= 0) port = 8888;

    int fd = client_connect(host, port);
    if (fd < 0) {
        printf("Khong ket noi duoc server.\n");
        return 1;
    }

    /* Doc READY neu co */
    char ready[MAX_LINE_LEN];
    client_recv_line_timeout(fd, ready, sizeof(ready), 500);

    struct NetBackend backend = { .fd = fd };
    struct UiBackendOps ops = {
        .user_data = &backend,
        .scan = backend_scan,
        .connect = backend_connect,
        .info = backend_info,
        .control = backend_control,
        .setcfg = backend_setcfg,
        .add_device = backend_add_device,
        .assign = backend_assign,
        .coop_list = backend_coop_list,
        .coop_add = backend_coop_add
    };

    struct UiContext ui;
    ui_context_init(&ui, &ops);
    (void)backend_coop_list(&backend, &ui.coop_list);
    ui_run(&ui);

    client_disconnect(fd);
    return 0;
}
