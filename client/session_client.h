#ifndef SESSION_CLIENT_H
#define SESSION_CLIENT_H

#include "config.h"

// Struct cho device ở client
struct ClientDevice {
    char id[MAX_ID_LEN];
    char token[MAX_TOKEN_LEN];
    int connected;  // 1 nếu đã connect
};

// Hàm lưu token sau CONNECT
void set_device_token(struct ClientDevice *dev, const char *token);

// Hàm lấy token
const char *get_device_token(struct ClientDevice *dev);

// Hàm reset khi BYE
void reset_device_session(struct ClientDevice *dev);

#endif 