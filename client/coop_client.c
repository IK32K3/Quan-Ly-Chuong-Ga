#include "coop_client.h"

#include <stdio.h>
#include <string.h>

void coop_list_init(struct CoopList *list) {
    if (!list) {
        return;
    }
    memset(list, 0, sizeof(*list));
}

int coop_add(struct CoopList *list, const char *name) {
    if (!list || !name) {
        return -1;
    }
    if (list->count >= MAX_COOPS) {
        return -2;
    }
    struct Coop *c = &list->coops[list->count];
    memset(c, 0, sizeof(*c));
    strncpy(c->name, name, sizeof(c->name) - 1);
    list->count++;
    return (int)(list->count - 1);
}

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

void coop_print(const struct CoopList *list) {
    if (!list) {
        return;
    }
    printf("Danh sach chuong nuoi:\n");
    for (size_t i = 0; i < list->count; ++i) {
        const struct Coop *c = &list->coops[i];
        printf("%zu) %s\n", i + 1, c->name);
        for (size_t j = 0; j < c->device_count; ++j) {
            const struct CoopDevice *d = &c->devices[j];
            printf("   - %s (%s)\n", d->device_id, device_type_to_string(d->type));
        }
    }
}
