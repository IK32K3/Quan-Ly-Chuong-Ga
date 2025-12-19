#ifndef SERVER_COOPS_H
#define SERVER_COOPS_H

#include <stddef.h>
#include "../shared/config.h"

#ifndef MAX_COOP_NAME
#define MAX_COOP_NAME 64
#endif

struct CoopMeta {
    int id;
    char name[MAX_COOP_NAME];
};

struct CoopsContext {
    struct CoopMeta coops[MAX_COOPS];
    size_t count;
    int next_id;
};

void coops_init(struct CoopsContext *ctx);
void coops_init_default(struct CoopsContext *ctx);
const struct CoopMeta *coops_find(const struct CoopsContext *ctx, int id);
int coops_upsert(struct CoopsContext *ctx, int id, const char *name);
int coops_add(struct CoopsContext *ctx, const char *name, int *out_id);

#endif /* SERVER_COOPS_H */
