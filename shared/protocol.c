#include "protocol.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stddef.h>  
#include <stdlib.h>  


struct command_entry {
    const char *name;
    enum CommandType cmd;
};

static const struct command_entry COMMAND_TABLE[] = {
    { "SCAN", CMD_SCAN },
    { "CONNECT", CMD_CONNECT },
    { "INFO", CMD_INFO },
    { "CONTROL", CMD_CONTROL },
    { "SETCFG", CMD_SETCFG },
    { "CHPASS", CMD_CHPASS },
    { "BYE", CMD_BYE },
    { "ADD", CMD_ADD_DEVICE },
    { "ADDDEVICE", CMD_ADD_DEVICE },
    { "ASSIGN", CMD_ASSIGN_DEVICE },
    { "SETCOOP", CMD_ASSIGN_DEVICE },
    { "COOPLIST", CMD_COOP_LIST },
    { "COOP_ADD", CMD_COOP_ADD },
    { "COOPADD", CMD_COOP_ADD }
};

enum CommandType protocol_command_from_string(const char *word) {
    if (!word) {
        return CMD_UNKNOWN;
    }

    for (size_t i = 0; i < sizeof(COMMAND_TABLE) / sizeof(COMMAND_TABLE[0]); ++i) {
        if (strcasecmp(word, COMMAND_TABLE[i].name) == 0) {
            return COMMAND_TABLE[i].cmd;
        }
    }

    return CMD_UNKNOWN;
}

const char *protocol_command_to_string(enum CommandType cmd) {
    switch (cmd) {
    case CMD_SCAN: return "SCAN";
    case CMD_CONNECT: return "CONNECT";
    case CMD_INFO: return "INFO";
    case CMD_CONTROL: return "CONTROL";
    case CMD_SETCFG: return "SETCFG";
    case CMD_CHPASS: return "CHPASS";
    case CMD_BYE: return "BYE";
    case CMD_ADD_DEVICE: return "ADD";
    case CMD_ASSIGN_DEVICE: return "ASSIGN";
    case CMD_COOP_LIST: return "COOPLIST";
    case CMD_COOP_ADD: return "COOPADD";
    default: return "UNKNOWN";
    }
}

int protocol_format_line(char *out, size_t len, int code, const char *text, const char *payload) {
    if (!out || len == 0 || !text) {
        return -1;
    }

    int written;
    if (payload && payload[0] != '\0') {
        written = snprintf(out, len, "%d %s %s", code, text, payload);
    } else {
        written = snprintf(out, len, "%d %s", code, text);
    }

    if (written < 0 || (size_t)written >= len) {
        return -1;
    }

    return 0;
}

int protocol_format_ready(char *out, size_t len) {
    return protocol_format_line(out, len, RESP_READY, "SERVER_READY", NULL);
}

int protocol_format_device(char *out, size_t len, const char *id, enum DeviceType type) {
    if (!id) {
        return -1;
    }
    char payload[MAX_LINE_LEN];
    int written = snprintf(payload, sizeof(payload), "DEVICE %s %s", id, device_type_to_string(type));
    if (written < 0 || (size_t)written >= sizeof(payload)) {
        return -1;
    }
    return protocol_format_line(out, len, RESP_DEVICE, payload, NULL);
}

int protocol_format_device_ex(char *out, size_t len, const char *id, enum DeviceType type, int coop_id) {
    if (!id) {
        return -1;
    }
    char payload[MAX_LINE_LEN];
    int written = snprintf(payload, sizeof(payload), "DEVICE %s %s %d", id, device_type_to_string(type), coop_id);
    if (written < 0 || (size_t)written >= sizeof(payload)) {
        return -1;
    }
    return protocol_format_line(out, len, RESP_DEVICE, payload, NULL);
}

int protocol_format_no_device_scan(char *out, size_t len) {
    return protocol_format_line(out, len, RESP_NO_DEVICE_SCAN, "NO_DEVICE", NULL);
}

int protocol_format_connect_ok(char *out, size_t len, const char *token) {
    return protocol_format_line(out, len, RESP_CONNECT_OK, "CONNECT_OK", token);
}

int protocol_format_wrong_password(char *out, size_t len) {
    return protocol_format_line(out, len, RESP_WRONG_PASSWORD, "WRONG_PASSWORD", NULL);
}

int protocol_format_no_device_err(char *out, size_t len) {
    return protocol_format_line(out, len, RESP_NO_DEVICE, "NO_DEVICE", NULL);
}

int protocol_format_info_ok(char *out, size_t len, const char *json) {
    return protocol_format_line(out, len, RESP_INFO_OK, "INFO_OK", json);
}

int protocol_format_control_ok(char *out, size_t len) {
    return protocol_format_line(out, len, RESP_CONTROL_OK, "CONTROL_OK", NULL);
}

int protocol_format_setcfg_ok(char *out, size_t len, const char *json) {
    return protocol_format_line(out, len, RESP_SETCFG_OK, "SETCFG_OK", json);
}

int protocol_format_pass_ok(char *out, size_t len) {
    return protocol_format_line(out, len, RESP_PASS_OK, "PASS_OK", NULL);
}

int protocol_format_bye_ok(char *out, size_t len) {
    return protocol_format_line(out, len, RESP_BYE_OK, "BYE_OK", NULL);
}

int protocol_format_add_ok(char *out, size_t len) {
    return protocol_format_line(out, len, RESP_ADD_OK, "ADD_OK", NULL);
}

int protocol_format_assign_ok(char *out, size_t len) {
    return protocol_format_line(out, len, RESP_ASSIGN_OK, "ASSIGN_OK", NULL);
}

int protocol_format_coop(char *out, size_t len, int coop_id, const char *name) {
    if (!name) return -1;
    char payload[MAX_LINE_LEN];
    int written = snprintf(payload, sizeof(payload), "COOP %d %s", coop_id, name);
    if (written < 0 || (size_t)written >= sizeof(payload)) return -1;
    return protocol_format_line(out, len, RESP_COOP, payload, NULL);
}

int protocol_format_coopadd_ok(char *out, size_t len, int coop_id) {
    char payload[32];
    int written = snprintf(payload, sizeof(payload), "%d", coop_id);
    if (written < 0 || (size_t)written >= sizeof(payload)) return -1;
    return protocol_format_line(out, len, RESP_COOPADD_OK, "COOPADD_OK", payload);
}

int protocol_format_no_coop(char *out, size_t len) {
    return protocol_format_line(out, len, RESP_NO_COOP, "NO_COOP", NULL);
}

int protocol_format_not_connected(char *out, size_t len) {
    return protocol_format_line(out, len, RESP_NOT_CONNECTED, "NOT_CONNECTED", NULL);
}

int protocol_format_bad_request(char *out, size_t len) {
    return protocol_format_line(out, len, RESP_BAD_REQUEST, "BAD_REQUEST", NULL);
}
