#include "protocol.h"
#include "types.h"
#include "config.h"
#include <stdio.h>
#include <string.h>
#include <strings.h>
#include <stddef.h>  
#include <stdlib.h>  

// Hàm parse response - THÊM MỚI
int protocol_parse_response(const char *line, int *code_out, char *text_out,
                           size_t text_len, char *payload_out, size_t payload_len) {
    if (!line || !code_out) return -1;
    
    // Parse code
    char *endptr;
    long code = strtol(line, &endptr, 10);
    if (endptr == line || code < 100) return -1;
    
    *code_out = (int)code;
    
    // Skip whitespace
    while (*endptr == ' ') endptr++;
    
    // Parse text
    const char *text_start = endptr;
    const char *text_end = strchr(text_start, ' ');
    
    if (text_out && text_len > 0) {
        if (text_end) {
            size_t copy_len = (size_t)(text_end - text_start);
            if (copy_len >= text_len) copy_len = text_len - 1;
            strncpy(text_out, text_start, copy_len);
            text_out[copy_len] = '\0';
        } else {
            strncpy(text_out, text_start, text_len - 1);
            text_out[text_len - 1] = '\0';
        }
    }
    
    // Parse payload (nếu có)
    if (text_end && payload_out && payload_len > 0) {
        const char *payload_start = text_end + 1;
        if (*payload_start) {
            strncpy(payload_out, payload_start, payload_len - 1);
            payload_out[payload_len - 1] = '\0';
        } else {
            payload_out[0] = '\0';
        }
    }
    
    return 0;
}

// Sửa hàm format_device
int protocol_format_device(char *out, size_t len, const char *id, enum DeviceType type) {
    if (!id || len == 0) return -1;
    
    int written = snprintf(out, len, "%d DEVICE %s %s", 
                          RESP_DEVICE, id, device_type_to_string(type));
    return (written < 0 || (size_t)written >= len) ? -1 : 0;
}

// Thêm hàm format mới
int protocol_format_already_connected(char *out, size_t len) {
    return protocol_format_line(out, len, 332, "ALREADY_CONNECTED", NULL);
}

int protocol_format_invalid_param(char *out, size_t len) {
    return protocol_format_line(out, len, 401, "INVALID_PARAM", NULL);
}

int protocol_format_internal_error(char *out, size_t len) {
    return protocol_format_line(out, len, 500, "INTERNAL_ERROR", NULL);
}