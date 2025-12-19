#include "session_auth.h"
#include "monitor_log.h"  // Thêm include cho log
#include <stdlib.h>
#include <string.h>
#include <time.h>

static struct Session sessions[MAX_DEVICES];

/**
 * @file session_auth.c
 * @brief Quản lý session token cho thiết bị sau khi CONNECT.
 *
 * Lưu ý: triển khai hiện tại dùng `rand()` và gọi `srand(time(NULL))` trong
 * `generate_token()`, phù hợp demo nhưng không phải mã an toàn cho production.
 */

/** @see generate_token() */
void generate_token(char *token, size_t len) {
    const char charset[] = "abcdefghijklmnopqrstuvwxyzABCDEFGHIJKLMNOPQRSTUVWXYZ0123456789";
    srand(time(NULL));
    for (size_t i = 0; i < len - 1; ++i) {
        token[i] = charset[rand() % (sizeof(charset) - 1)];
    }
    token[len - 1] = '\0';
}

/** @see create_session() */
int create_session(const char *device_id, char *token_out) {
    for (int i = 0; i < MAX_DEVICES; ++i) {
        if (!sessions[i].active) {
            generate_token(sessions[i].token, MAX_TOKEN_LEN);
            strcpy(sessions[i].device_id, device_id);
            sessions[i].active = 1;
            strcpy(token_out, sessions[i].token);
            log_device_event(device_id, "Session created");  // Thêm log
            return 0;
        }
    }
    return -1;  // No slot
}

/** @see validate_session() */
int validate_session(const char *token, char *device_id_out) {
    for (int i = 0; i < MAX_DEVICES; ++i) {
        if (sessions[i].active && strcmp(sessions[i].token, token) == 0) {
            strcpy(device_id_out, sessions[i].device_id);
            return 0;
        }
    }
    return -1;
}

/** @see end_session() */
void end_session(const char *token) {
    for (int i = 0; i < MAX_DEVICES; ++i) {
        if (sessions[i].active && strcmp(sessions[i].token, token) == 0) {
            log_device_event(sessions[i].device_id, "Session ended");  // Thêm log
            sessions[i].active = 0;
            break;
        }
    }
}
