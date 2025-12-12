#ifndef SHARED_CONFIG_H
#define SHARED_CONFIG_H

// Kích thước buffer
#define MAX_DEVICES 32
#define MAX_ID_LEN 32
#define MAX_TYPE_LEN 16
#define MAX_PASSWORD_LEN 64
#define MAX_TOKEN_LEN 64
#define MAX_LINE_LEN 2048        // TĂNG từ 1024
#define MAX_JSON_LEN 2048        // TĂNG từ 1024
#define MAX_ACTION_LEN 16
#define MAX_ADDR_LEN 64
#define LOG_LINE_LEN 256

// Cấu hình mạng
#define DEFAULT_PORT 8888
#define DEFAULT_BACKLOG 8
#define SERVER_IP "127.0.0.1"
#define TIMEOUT_SEC 5
#define MAX_RETRIES 3

// Quản lý session và chuồng
#define MAX_SESSIONS 32
#define MAX_COOPS 10
#define MAX_DEVICES_PER_COOP 10
#define MAX_CLIENTS 10

// Cấu hình thiết bị
#define MAX_SCHEDULE_ENTRIES 10
#define MAX_PARAM_NAME_LEN 32
#define MAX_PARAM_VALUE_LEN 64

// Giá trị mặc định cho thiết bị
#define DEFAULT_TEMP_MIN 20.0
#define DEFAULT_TEMP_MAX 30.0
#define DEFAULT_HUMIDITY_MIN 40.0
#define DEFAULT_HUMIDITY_MAX 70.0
#define DEFAULT_FEED_AMOUNT 2.0    // kg
#define DEFAULT_WATER_AMOUNT 5.0   // lít

// Debug
#ifdef DEBUG
#define LOG_DEBUG(fmt, ...) fprintf(stderr, "[DEBUG] " fmt "\n", ##__VA_ARGS__)
#else
#define LOG_DEBUG(fmt, ...)
#endif

#endif  /* SHARED_CONFIG_H */