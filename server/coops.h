#ifndef SERVER_COOPS_H
#define SERVER_COOPS_H

#include <stddef.h>
#include "../shared/config.h"

#ifndef MAX_COOP_NAME
#define MAX_COOP_NAME 64
#endif

/** @brief Metadata tối thiểu của một chuồng (ID + tên). */
struct CoopMeta {
    int id;
    char name[MAX_COOP_NAME];
};

/** @brief Context quản lý danh sách chuồng phía server. */
struct CoopsContext {
    struct CoopMeta coops[MAX_COOPS];
    size_t count;
    int next_id;
};

/** @brief Khởi tạo context chuồng (reset danh sách, set `next_id`). */
void coops_init(struct CoopsContext *ctx);

/**
 * @brief Tìm chuồng theo ID.
 * @return Con trỏ `CoopMeta` nếu tìm thấy, NULL nếu không có.
 */
const struct CoopMeta *coops_find(const struct CoopsContext *ctx, int id);

/**
 * @brief Thêm mới hoặc cập nhật (upsert) chuồng theo ID.
 * @return 0 nếu thành công, giá trị âm nếu lỗi (tham số sai, đầy,...).
 */
int coops_upsert(struct CoopsContext *ctx, int id, const char *name);

/**
 * @brief Thêm chuồng mới với ID tự tăng.
 * @return 0 nếu thành công, giá trị âm nếu lỗi.
 */
int coops_add(struct CoopsContext *ctx, const char *name, int *out_id);

#endif /* SERVER_COOPS_H */
