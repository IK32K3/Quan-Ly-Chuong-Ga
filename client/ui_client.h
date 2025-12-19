#ifndef CLIENT_UI_CLIENT_H
#define CLIENT_UI_CLIENT_H

#include <stddef.h>
#include "../shared/types.h"
#include "../shared/config.h"
#include "coop_client.h"

struct UiBackendOps {
    void *user_data;
    int (*scan)(void *user_data, struct DeviceIdentity *out, size_t max_out, size_t *found);
    int (*connect)(void *user_data, const char *device_id, const char *password, char *out_token, size_t token_len, enum DeviceType *out_type);
    int (*info)(void *user_data, const char *device_id, const char *token, char *out_json, size_t json_len);
    int (*control)(void *user_data, const char *device_id, const char *token, const char *action, const char *payload);
    int (*setcfg)(void *user_data, const char *device_id, const char *token, const char *json_payload);
    int (*add_device)(void *user_data, const char *device_id, enum DeviceType type, const char *password, int coop_id);
    int (*assign)(void *user_data, const char *device_id, int coop_id);
    int (*coop_list)(void *user_data, struct CoopList *out);
    int (*coop_add)(void *user_data, const char *name, int *out_id);
};

struct ClientDevice {
    char id[MAX_ID_LEN];
    char token[MAX_TOKEN_LEN];
    enum DeviceType type;
    int connected;
};

struct UiContext {
    struct UiBackendOps ops;
    struct ClientDevice devices[MAX_DEVICES];
    size_t device_count;
    struct CoopList coop_list;
};

void ui_context_init(struct UiContext *ctx, const struct UiBackendOps *ops);
void ui_run(struct UiContext *ctx);

#endif /* CLIENT_UI_CLIENT_H */
