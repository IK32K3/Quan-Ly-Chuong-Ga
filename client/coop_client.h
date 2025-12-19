#ifndef CLIENT_COOP_CLIENT_H
#define CLIENT_COOP_CLIENT_H

#include <stddef.h>
#include "../shared/types.h"
#include "../shared/config.h"

#define MAX_COOP_NAME 64

/** @brief Một thiết bị nằm trong chuồng (định danh + type). */
struct CoopDevice {
    char device_id[MAX_ID_LEN];
    enum DeviceType type;
};

/** @brief Thông tin chuồng phía client (để hiển thị UI). */
struct Coop {
    int id;
    char name[MAX_COOP_NAME];
    struct CoopDevice devices[MAX_DEVICES];
    size_t device_count;
};

/** @brief Danh sách chuồng (tối đa `MAX_COOPS`). */
struct CoopList {
    struct Coop coops[MAX_COOPS];
    size_t count;
};

/** @brief Khởi tạo `CoopList` về rỗng. */
void coop_list_init(struct CoopList *list);

/**
 * @brief Thêm chuồng mới vào danh sách phía client.
 *
 * @return Index của chuồng mới trong mảng nếu thành công (>=0),
 *         giá trị âm nếu lỗi (đầy hoặc tham số sai).
 */
int coop_add(struct CoopList *list, const char *name);

/**
 * @brief Thêm một thiết bị vào chuồng theo index.
 *
 * @return 0 nếu thành công, giá trị âm nếu lỗi (index sai, trùng ID, đầy,...).
 */
int coop_add_device(struct CoopList *list, size_t coop_index, const char *device_id, enum DeviceType type);

/** @brief In danh sách chuồng và thiết bị ra stdout. */
void coop_print(const struct CoopList *list);

#endif /* CLIENT_COOP_CLIENT_H */
