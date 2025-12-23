#include "coops.h"

#include <string.h>

/**
 * @brief Quản lý danh sách chuồng (thêm/tìm/cập nhật) phía server.
 */

void coops_init(struct CoopsContext *ctx) {
    if (!ctx) return;
    memset(ctx, 0, sizeof(*ctx));
    ctx->next_id = 1;
}

const struct CoopMeta *coops_find(const struct CoopsContext *ctx, int id) {
    if (!ctx) return NULL;
    for (size_t i = 0; i < ctx->count; ++i) {
        if (ctx->coops[i].id == id) return &ctx->coops[i];
    }
    return NULL;
}

int coops_upsert(struct CoopsContext *ctx, int id, const char *name) {
    if (!ctx || id <= 0 || !name || name[0] == '\0') return -1;
    for (size_t i = 0; i < ctx->count; ++i) {
        if (ctx->coops[i].id == id) {
            strncpy(ctx->coops[i].name, name, sizeof(ctx->coops[i].name) - 1);
            ctx->coops[i].name[sizeof(ctx->coops[i].name) - 1] = '\0';
            if (id >= ctx->next_id) ctx->next_id = id + 1;
            return 0;
        }
    }
    if (ctx->count >= MAX_COOPS) return -2;
    struct CoopMeta *c = &ctx->coops[ctx->count++];
    memset(c, 0, sizeof(*c));
    c->id = id;
    strncpy(c->name, name, sizeof(c->name) - 1);
    c->name[sizeof(c->name) - 1] = '\0';
    if (id >= ctx->next_id) ctx->next_id = id + 1;
    return 0;
}

int coops_add(struct CoopsContext *ctx, const char *name, int *out_id) {
    if (!ctx || !name || name[0] == '\0') return -1;
    if (ctx->count >= MAX_COOPS) return -2;
    int id = ctx->next_id <= 0 ? 1 : ctx->next_id;
    if (coops_upsert(ctx, id, name) != 0) return -3;
    if (out_id) *out_id = id;
    return 0;
}
