#ifndef CLIENT_COOP_CLIENT_H
#define CLIENT_COOP_CLIENT_H

#include <stddef.h>
#include "../shared/types.h"
#include "../shared/config.h"

#define MAX_COOPS 8
#define MAX_COOP_NAME 64

struct CoopDevice {
    char device_id[MAX_ID_LEN];
    enum DeviceType type;
};

struct Coop {
    char name[MAX_COOP_NAME];
    struct CoopDevice devices[MAX_DEVICES];
    size_t device_count;
};

struct CoopList {
    struct Coop coops[MAX_COOPS];
    size_t count;
};

void coop_list_init(struct CoopList *list);
int coop_add(struct CoopList *list, const char *name);
int coop_add_device(struct CoopList *list, size_t coop_index, const char *device_id, enum DeviceType type);
void coop_print(const struct CoopList *list);

#endif /* CLIENT_COOP_CLIENT_H */
