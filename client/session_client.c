#include "session_client.h"
#include <string.h>

void set_device_token(struct ClientDevice *dev, const char *token) {
    strcpy(dev->token, token);
    dev->connected = 1;
}

const char *get_device_token(struct ClientDevice *dev) {
    return dev->connected ? dev->token : NULL;
}

void reset_device_session(struct ClientDevice *dev) {
    dev->token[0] = '\0';
    dev->connected = 0;
}