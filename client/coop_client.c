#include "coop_client.h"

#include <string.h>

void coop_list_init(struct CoopList *list) {
    if (!list) {
        return;
    }
    memset(list, 0, sizeof(*list));
}

/** @brief Kiểm tra chuồng đã có thiết bị với ID này hay chưa. */
static int device_exists(const struct Coop *c, const char *device_id) {
    for (size_t i = 0; i < c->device_count; ++i) {
        if (strncmp(c->devices[i].device_id, device_id, sizeof(c->devices[i].device_id)) == 0) {
            return 1;
        }
    }
    return 0;
}

int coop_add_device(struct CoopList *list, size_t coop_index, const char *device_id, enum DeviceType type) {
    if (!list || !device_id) {
        return -1;
    }
    if (coop_index >= list->count) {
        return -2;
    }
    struct Coop *c = &list->coops[coop_index];
    if (c->device_count >= MAX_DEVICES) {
        return -3;
    }
    if (device_exists(c, device_id)) {
        return -4;
    }
    struct CoopDevice *d = &c->devices[c->device_count++];
    memset(d, 0, sizeof(*d));
    strncpy(d->device_id, device_id, sizeof(d->device_id) - 1);
    d->type = type;
    return 0;
}
