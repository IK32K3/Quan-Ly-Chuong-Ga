#ifndef SERVER_STORAGE_H
#define SERVER_STORAGE_H

#include "devices.h"
#include "coops.h"

/**
 * @brief Lưu danh sách thiết bị ra file JSON (dạng array).
 *
 * @return 0 nếu thành công, -1 nếu lỗi I/O hoặc tham số sai.
 */
int storage_save_devices(const struct DevicesContext *ctx, const char *path);

/**
 * @brief Tải danh sách thiết bị từ file JSON.
 *
 * @return 0 nếu tải được, -1 nếu lỗi/không có file/không parse được.
 */
int storage_load_devices(struct DevicesContext *ctx, const char *path);

/**
 * @brief Lưu toàn bộ farm (chuồng + thiết bị) ra file JSON.
 *
 * Format: object { "coops": [ {id, name, devices:[{password, info}, ...]}, ...] }
 *
 * @return 0 nếu thành công, -1 nếu lỗi.
 */
int storage_save_farm(const struct CoopsContext *coops, const struct DevicesContext *devices, const char *path);

/**
 * @brief Tải toàn bộ farm (chuồng + thiết bị) từ file JSON.
 *
 * @return 0 nếu thành công, -1 nếu lỗi/không có file.
 */
int storage_load_farm(struct CoopsContext *coops, struct DevicesContext *devices, const char *path);

#endif /* SERVER_STORAGE_H */
