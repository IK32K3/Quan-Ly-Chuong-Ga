#ifndef SERVER_STORAGE_H
#define SERVER_STORAGE_H

#include "devices.h"
#include "coops.h"

/* Luu danh sach thiet bi ra file JSON */
int storage_save_devices(const struct DevicesContext *ctx, const char *path);

/* Tai danh sach thiet bi tu file JSON. Tra 0 neu tai duoc, -1 neu loi/khong co file. */
int storage_load_devices(struct DevicesContext *ctx, const char *path);

/* Farm storage (chuong + thiet bi). */
int storage_save_farm(const struct CoopsContext *coops, const struct DevicesContext *devices, const char *path);
int storage_load_farm(struct CoopsContext *coops, struct DevicesContext *devices, const char *path);

#endif /* SERVER_STORAGE_H */
