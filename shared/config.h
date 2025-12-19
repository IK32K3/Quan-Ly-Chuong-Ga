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

// Cấu hình mạng
#define DEFAULT_PORT 8888
#define DEFAULT_BACKLOG 8

// Quản lý session và chuồng
#define MAX_COOPS 10

// Cấu hình thiết bị
#define MAX_SCHEDULE_ENTRIES 10

#endif  /* SHARED_CONFIG_H */
