#include "ui_client.h"

#include <jansson.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helpers cho UI: tra cuu, nhap lieu va parse JSON */

/**
 * @file ui_client.c
 * @brief UI dạng menu trên terminal cho client (gọi backend qua callback).
 */

/** @brief Tìm thiết bị trong context client theo ID. */
static const struct ClientDevice *find_client_device_const(const struct UiContext *ctx, const char *id) {
    if (!ctx || !id) {
        return NULL;
    }
    for (size_t i = 0; i < ctx->device_count; ++i) {
        if (strncmp(ctx->devices[i].id, id, sizeof(ctx->devices[i].id)) == 0) {
            return &ctx->devices[i];
        }
    }
    return NULL;
}

static struct ClientDevice *find_client_device(struct UiContext *ctx, const char *id) {
    return (struct ClientDevice *)find_client_device_const(ctx, id);
}

/** @brief In danh sách chuồng và chỉ hiển thị các thiết bị đã CONNECT. */
static void print_coops_with_connection(const struct UiContext *ctx) {
    if (!ctx) return;
    printf("Danh sach chuong nuoi:\n");
    for (size_t i = 0; i < ctx->coop_list.count; ++i) {
        const struct Coop *c = &ctx->coop_list.coops[i];
        printf("%zu) %s\n", i + 1, c->name);
        size_t printed = 0;
        for (size_t j = 0; j < c->device_count; ++j) {
            const struct CoopDevice *d = &c->devices[j];
            const struct ClientDevice *cd = find_client_device_const(ctx, d->device_id);
            if (!(cd && cd->connected)) {
                continue;
            }
            printf("   - %s (%s)\n", d->device_id, device_type_to_string(d->type));
            printed++;
        }
        if (printed == 0) {
            printf("   (Chua co thiet bi da ket noi)\n");
        }
    }
}

/**
 * @brief Lấy (hoặc tạo) entry thiết bị trong danh sách client.
 *
 * Hàm không tự CONNECT; chỉ đảm bảo có slot để lưu token/trạng thái.
 */
static struct ClientDevice *ensure_client_device(struct UiContext *ctx, const char *id, enum DeviceType type) {
    struct ClientDevice *existing = find_client_device(ctx, id);
    if (existing) {
        return existing;
    }
    if (ctx->device_count >= MAX_DEVICES) {
        return NULL;
    }
    struct ClientDevice *d = &ctx->devices[ctx->device_count++];
    memset(d, 0, sizeof(*d));
    strncpy(d->id, id, sizeof(d->id) - 1);
    d->type = type;
    return d;
}

/** @brief Đọc một dòng từ stdin và loại bỏ ký tự xuống dòng. */
static int read_line(char *buf, size_t len) {
    if (!fgets(buf, (int)len, stdin)) {
        return -1;
    }
    size_t l = strlen(buf);
    if (l > 0 && buf[l - 1] == '\n') {
        buf[l - 1] = '\0';
    }
    return 0;
}

/** @brief Hỏi người dùng nhập số thực (double) theo prompt. */
static int read_double(const char *prompt, double *out) {
    char line[64];
    printf("%s", prompt);
    if (read_line(line, sizeof(line)) != 0) {
        return -1;
    }
    char *endptr = NULL;
    double v = strtod(line, &endptr);
    if (endptr == line) {
        return -1;
    }
    *out = v;
    return 0;
}

/** @brief Kiểm tra type có hỗ trợ ON/OFF hay không. */
static int device_supports_power(enum DeviceType type) {
    switch (type) {
    case DEVICE_FAN:
    case DEVICE_HEATER:
    case DEVICE_SPRAYER:
    case DEVICE_FEEDER:
    case DEVICE_DRINKER:
        return 1;
    default:
        return 0;
    }
}

/** @brief Cho người dùng chọn 1 chuồng trong `CoopList`. */
static int select_coop_index(const struct CoopList *list, size_t *out_index) {
    if (list->count == 0) {
        printf("Chua co chuong nao. Hay them chuong truoc.\n");
        return -1;
    }
    printf("Danh sach chuong nuoi:\n");
    for (size_t i = 0; i < list->count; ++i) {
        printf("%zu) %s\n", i + 1, list->coops[i].name);
    }
    printf("Chon so thu tu chuong (0 de quay lai): ");
    char line[16];
    if (read_line(line, sizeof(line)) != 0) return -1;
    long idx = strtol(line, NULL, 10);
    if (idx == 0) {
        return -1;
    }
    if (idx < 0 || (size_t)idx > list->count) {
        printf("So thu tu khong hop le.\n");
        return -1;
    }
    *out_index = (size_t)(idx - 1);
    return 0;
}

/** @brief Đồng bộ danh sách chuồng và gán thiết bị vào chuồng dựa trên kết quả SCAN. */
static void refresh_coops(struct UiContext *ctx) {
    if (!ctx || !ctx->ops.coop_list || !ctx->ops.scan) return;
    if (ctx->ops.coop_list(ctx->ops.user_data, &ctx->coop_list) != 0) {
        return;
    }
    for (size_t i = 0; i < ctx->coop_list.count; ++i) {
        ctx->coop_list.coops[i].device_count = 0;
    }

    struct DeviceIdentity devices[MAX_DEVICES];
    size_t found = 0;
    if (ctx->ops.scan(ctx->ops.user_data, devices, MAX_DEVICES, &found) != 0 || found == 0) {
        return;
    }

    for (size_t i = 0; i < found; ++i) {
        for (size_t j = 0; j < ctx->coop_list.count; ++j) {
            if (devices[i].coop_id == ctx->coop_list.coops[j].id) {
                coop_add_device(&ctx->coop_list, j, devices[i].id, devices[i].type);
                break;
            }
        }
    }
}

static int select_device_in_coop_impl(const struct UiContext *ctx,
                                      size_t coop_index,
                                      int require_connected,
                                      char *out_id,
                                      size_t out_id_len,
                                      enum DeviceType *out_type) {
    if (!ctx || coop_index >= ctx->coop_list.count || !out_id || out_id_len == 0) return -1;
    const struct Coop *c = &ctx->coop_list.coops[coop_index];
    if (c->device_count == 0) {
        printf("Chua co thiet bi nao trong chuong nay.\n");
        return -1;
    }

    size_t mapped[MAX_DEVICES];
    size_t mapped_count = 0;
    for (size_t i = 0; i < c->device_count && mapped_count < MAX_DEVICES; ++i) {
        const struct ClientDevice *cd = find_client_device_const(ctx, c->devices[i].device_id);
        if (require_connected && !(cd && cd->connected)) {
            continue;
        }
        mapped[mapped_count++] = i;
    }
    if (mapped_count == 0) {
        if (require_connected) {
            printf("Chua ket noi thiet bi nao trong chuong %s.\n", c->name);
        } else {
            printf("Chua co thiet bi nao trong chuong nay.\n");
        }
        return -1;
    }

    printf("Thiet bi trong chuong %s:\n", c->name);
    for (size_t i = 0; i < mapped_count; ++i) {
        const struct CoopDevice *d = &c->devices[mapped[i]];
        const struct ClientDevice *cd = find_client_device_const(ctx, d->device_id);
        printf("%zu) %s (%s)%s\n",
               i + 1,
               d->device_id,
               device_type_to_string(d->type),
               (!require_connected && cd && cd->connected) ? " [DA KET NOI]" : "");
    }

    printf("Chon thiet bi (0 de quay lai): ");
    char line[16];
    if (read_line(line, sizeof(line)) != 0) return -1;
    long pick = strtol(line, NULL, 10);
    if (pick == 0) return -1;
    if (pick < 0 || (size_t)pick > mapped_count) {
        printf("Lua chon khong hop le.\n");
        return -1;
    }

    const struct CoopDevice *d = &c->devices[mapped[pick - 1]];
    strncpy(out_id, d->device_id, out_id_len - 1);
    out_id[out_id_len - 1] = '\0';
    if (out_type) *out_type = d->type;
    return 0;
}

/** @brief Chọn một thiết bị bất kỳ trong chuồng (có thể chưa CONNECT). */
static int select_device_in_coop(const struct UiContext *ctx, size_t coop_index, char *out_id, size_t out_id_len, enum DeviceType *out_type) {
    return select_device_in_coop_impl(ctx, coop_index, 0, out_id, out_id_len, out_type);
}

/** @brief Chọn một thiết bị đã CONNECT trong chuồng. */
static int select_connected_device_in_coop(const struct UiContext *ctx, size_t coop_index, char *out_id, size_t out_id_len, enum DeviceType *out_type) {
    return select_device_in_coop_impl(ctx, coop_index, 1, out_id, out_id_len, out_type);
}

static const char *json_get_string_or_empty(json_t *obj, const char *key) {
    json_t *v = json_object_get(obj, key);
    return json_is_string(v) ? json_string_value(v) : "";
}

static double json_get_number_or_zero(json_t *obj, const char *key) {
    json_t *v = json_object_get(obj, key);
    return json_is_number(v) ? json_number_value(v) : 0.0;
}

/* Hien thi thong tin thiet bi theo tung type*/
/** @brief Hiển thị JSON INFO theo từng loại thiết bị. */
static void display_info(const char *json) {
    json_t *root = json_loads(json, 0, NULL);
    if (!json_is_object(root)) {
        if (root) json_decref(root);
        printf("Khong doc duoc thong tin thiet bi.\n");
        return;
    }

    const char *type = json_get_string_or_empty(root, "type");
    const char *id = json_get_string_or_empty(root, "device_id");
    if (type[0] == '\0' || id[0] == '\0') {
        json_decref(root);
        printf("Khong doc duoc thong tin thiet bi.\n");
        return;
    }

    printf("Thiet bi: %s (%s)\n", id, type);
    if (strcmp(type, "sensor") == 0) {
        double t = json_get_number_or_zero(root, "temperature");
        double h = json_get_number_or_zero(root, "humidity");
        const char *ut = json_get_string_or_empty(root, "unit_temperature");
        const char *uh = json_get_string_or_empty(root, "unit_humidity");
        printf("Nhiet do: %.1f %s\n", t, ut);
        printf("Do am: %.1f %s\n", h, uh);
    } else if (strcmp(type, "fan") == 0) {
        const char *state = json_get_string_or_empty(root, "state");
        double nhiet_do_bat_c = json_get_number_or_zero(root, "nhiet_do_bat_c");
        double nhiet_do_tat_c = json_get_number_or_zero(root, "nhiet_do_tat_c");
        const char *unit = json_get_string_or_empty(root, "unit_temp");
        printf("Trang thai: %s\n", state);
        printf("Nhiet do bat: %.1f %s | Nhiet do tat: %.1f %s\n", nhiet_do_bat_c, unit, nhiet_do_tat_c, unit);
    } else if (strcmp(type, "heater") == 0) {
        const char *state = json_get_string_or_empty(root, "state");
        double nhiet_do_bat_c = json_get_number_or_zero(root, "nhiet_do_bat_c");
        double nhiet_do_tat_c = json_get_number_or_zero(root, "nhiet_do_tat_c");
        const char *mode = json_get_string_or_empty(root, "mode");
        const char *unit = json_get_string_or_empty(root, "unit_temp");
        printf("Trang thai: %s (mode %s)\n", state, mode);
        printf("Nhiet do bat: %.1f %s | Nhiet do tat: %.1f %s\n", nhiet_do_bat_c, unit, nhiet_do_tat_c, unit);
    } else if (strcmp(type, "sprayer") == 0) {
        const char *state = json_get_string_or_empty(root, "state");
        double do_am_bat_pct = json_get_number_or_zero(root, "do_am_bat_pct");
        double do_am_muc_tieu_pct = json_get_number_or_zero(root, "do_am_muc_tieu_pct");
        double luu_luong_lph = json_get_number_or_zero(root, "luu_luong_lph");
        const char *uh = json_get_string_or_empty(root, "unit_humidity");
        const char *uf = json_get_string_or_empty(root, "unit_flow");
        printf("Trang thai: %s\n", state);
        printf("Nguong do am bat/muc tieu: %.1f/%.1f %s\n", do_am_bat_pct, do_am_muc_tieu_pct, uh);
        printf("Luu luong phun: %.1f %s\n", luu_luong_lph, uf);
    } else if (strcmp(type, "feeder") == 0) {
        const char *state = json_get_string_or_empty(root, "state");
        double thuc_an_kg = json_get_number_or_zero(root, "thuc_an_kg");
        double nuoc_l = json_get_number_or_zero(root, "nuoc_l");
        const char *uf = json_get_string_or_empty(root, "unit_food");
        const char *uw = json_get_string_or_empty(root, "unit_water");
        printf("Trang thai: %s\n", state);
        printf("Suat an: %.1f %s, Nuoc: %.1f %s\n", thuc_an_kg, uf, nuoc_l, uw);
    } else if (strcmp(type, "drinker") == 0) {
        const char *state = json_get_string_or_empty(root, "state");
        double nuoc_l = json_get_number_or_zero(root, "nuoc_l");
        const char *uw = json_get_string_or_empty(root, "unit_water");
        printf("Trang thai: %s\n", state);
        printf("Luong nuoc moi lan: %.1f %s\n", nuoc_l, uw);
    } else if (strcmp(type, "egg_counter") == 0) {
        json_t *eggs = json_object_get(root, "egg_count");
        if (json_is_integer(eggs)) {
            printf("So trung dem duoc: %lld\n", (long long)json_integer_value(eggs));
        } else if (json_is_number(eggs)) {
            printf("So trung dem duoc: %d\n", (int)json_number_value(eggs));
        }
    } else {
        printf("Loai thiet bi chua ho tro hien thi chi tiet.\n");
    }

    json_decref(root);
}

/** @brief Menu: CONNECT thiết bị (chọn từ chuồng -> nhập mật khẩu). */
static void menu_connect(struct UiContext *ctx) {
    refresh_coops(ctx);
    size_t coop_idx = 0;
    if (select_coop_index(&ctx->coop_list, &coop_idx) != 0) {
        return;
    }
    char id[MAX_ID_LEN];
    enum DeviceType type = DEVICE_UNKNOWN;
    if (select_device_in_coop(ctx, coop_idx, id, sizeof(id), &type) != 0) {
        return;
    }
    char pw[MAX_PASSWORD_LEN];
    printf("Nhap mat khau: ");
    if (read_line(pw, sizeof(pw)) != 0) {
        return;
    }
    char token[MAX_TOKEN_LEN];
    if (ctx->ops.connect(ctx->ops.user_data, id, pw, token, sizeof(token), &type) != 0) {
        printf("Ket noi that bai (sai mat khau hoac khong ton tai).\n");
        return;
    }
    struct ClientDevice *d = ensure_client_device(ctx, id, type);
    if (!d) {
        printf("Khong the luu thong tin thiet bi (bang day).\n");
        return;
    }
    strncpy(d->token, token, sizeof(d->token) - 1);
    d->connected = 1;
    d->type = type;
    printf("Ket noi %s thanh cong!\n", id);
}

/** @brief Yêu cầu thiết bị đã CONNECT trước khi thao tác cần token. */
static const struct ClientDevice *require_connected(struct UiContext *ctx, const char *id) {
    struct ClientDevice *d = find_client_device(ctx, id);
    if (!d || !d->connected) {
        printf("Chua ket noi %s. Hay ket noi truoc.\n", id);
        return NULL;
    }
    return d;
}

static const struct ClientDevice *select_connected_device(struct UiContext *ctx, char *out_id, size_t out_id_len) {
    if (!ctx || !out_id || out_id_len == 0) return NULL;
    refresh_coops(ctx);
    size_t coop_idx = 0;
    if (select_coop_index(&ctx->coop_list, &coop_idx) != 0) {
        return NULL;
    }
    enum DeviceType type = DEVICE_UNKNOWN;
    if (select_connected_device_in_coop(ctx, coop_idx, out_id, out_id_len, &type) != 0) {
        return NULL;
    }
    return require_connected(ctx, out_id);
}

/* Menu xem thong tin: chi cho phep thiet bi da ket noi trong chuong */
/** @brief Menu: INFO thiết bị đã CONNECT. */
static void menu_info(struct UiContext *ctx) {
    char id[MAX_ID_LEN];
    const struct ClientDevice *d = select_connected_device(ctx, id, sizeof(id));
    if (!d) {
        return;
    }
    char json[MAX_JSON_LEN];
    if (ctx->ops.info(ctx->ops.user_data, id, d->token, json, sizeof(json)) != 0) {
        printf("Khong lay duoc thong tin %s.\n", id);
        return;
    }
    printf("Thong tin thiet bi:\n");
    display_info(json);
}

/* Dieu khien thiet bi: ON/OFF hoac hanh dong truc tiep (feeder/drinker/sprayer) */
/** @brief Menu: CONTROL thiết bị đã CONNECT (ON/OFF hoặc hành động trực tiếp). */
static void menu_control(struct UiContext *ctx, int direct_mode) {
    char id[MAX_ID_LEN];
    const struct ClientDevice *d = select_connected_device(ctx, id, sizeof(id));
    if (!d) {
        return;
    }
    if (!direct_mode && !device_supports_power(d->type)) {
        printf("Thiet bi nay khong ho tro bat/tat.\n");
        return;
    }

    char action[MAX_ACTION_LEN] = {0};
    char payload[MAX_JSON_LEN] = {0};

    if (direct_mode && d->type == DEVICE_FEEDER) {
        double food = 0.0, water = 0.0;
        if (read_double("Nhap luong thuc an (kg): ", &food) != 0) return;
        if (read_double("Nhap luong nuoc (L): ", &water) != 0) return;
        strncpy(action, "FEED_NOW", sizeof(action) - 1);
        snprintf(payload, sizeof(payload), "{\"thuc_an_kg\":%.1f,\"nuoc_l\":%.1f}", food, water);
    } else if (direct_mode && d->type == DEVICE_DRINKER) {
        double water = 0.0;
        if (read_double("Nhap luong nuoc (L): ", &water) != 0) return;
        strncpy(action, "DRINK_NOW", sizeof(action) - 1);
        snprintf(payload, sizeof(payload), "{\"nuoc_l\":%.1f}", water);
    } else if (direct_mode && d->type == DEVICE_SPRAYER) {
        double Vh = 0.0;
        if (read_double("Nhap luu luong phun (L/h): ", &Vh) != 0) return;
        strncpy(action, "SPRAY_NOW", sizeof(action) - 1);
        snprintf(payload, sizeof(payload), "{\"luu_luong_lph\":%.1f}", Vh);
    } else {
        printf("Chon hanh dong (1=ON, 0=OFF): ");
        char line[8];
        if (read_line(line, sizeof(line)) != 0) return;
        int on = atoi(line);
        strncpy(action, on ? "ON" : "OFF", sizeof(action) - 1);
    }

    if (ctx->ops.control(ctx->ops.user_data, id, d->token, action, payload[0] ? payload : NULL) != 0) {
        printf("Dieu khien that bai.\n");
        return;
    }
    printf("Da gui lenh %s cho %s.\n", action, id);
}

/* Thiet lap tham so theo tung loai thiet bi */
/** @brief Menu: SETCFG thiết bị đã CONNECT (tuỳ theo type). */
static void menu_setcfg(struct UiContext *ctx) {
    char id[MAX_ID_LEN];
    const struct ClientDevice *d = select_connected_device(ctx, id, sizeof(id));
    if (!d) {
        return;
    }

    char json[MAX_JSON_LEN] = {0};
    switch (d->type) {
    case DEVICE_FAN: {
        double Tmax, Tp1;
        if (read_double("Nhap nhiet do bat quat (do C): ", &Tmax) != 0) return;
        if (read_double("Nhap nhiet do tat quat (do C): ", &Tp1) != 0) return;
        snprintf(json, sizeof(json), "{\"nhiet_do_bat_c\":%.1f,\"nhiet_do_tat_c\":%.1f}", Tmax, Tp1);
        break;
    }
    case DEVICE_HEATER: {
        double Tmin, Tp2;
        if (read_double("Nhap nhiet do bat den suoi (do C): ", &Tmin) != 0) return;
        if (read_double("Nhap nhiet do tat den suoi (do C): ", &Tp2) != 0) return;
        snprintf(json, sizeof(json), "{\"nhiet_do_bat_c\":%.1f,\"nhiet_do_tat_c\":%.1f}", Tmin, Tp2);
        break;
    }
    case DEVICE_SPRAYER: {
        double Hmin, Hp, Vh;
        if (read_double("Nhap do am bat phun (%%): ", &Hmin) != 0) return;
        if (read_double("Nhap do am muc tieu (%%): ", &Hp) != 0) return;
        if (read_double("Nhap luu luong phun (L/h): ", &Vh) != 0) return;
        snprintf(json, sizeof(json), "{\"do_am_bat_pct\":%.1f,\"do_am_muc_tieu_pct\":%.1f,\"luu_luong_lph\":%.1f}", Hmin, Hp, Vh);
        break;
    }
    case DEVICE_FEEDER: {
        double W, Vw;
        if (read_double("Nhap khau phan an moi lan (kg): ", &W) != 0) return;
        if (read_double("Nhap luong nuoc moi lan (L): ", &Vw) != 0) return;
        snprintf(json, sizeof(json), "{\"thuc_an_kg\":%.1f,\"nuoc_l\":%.1f}", W, Vw);
        break;
    }
    case DEVICE_DRINKER: {
        double Vw;
        if (read_double("Nhap luong nuoc moi lan (L): ", &Vw) != 0) return;
        snprintf(json, sizeof(json), "{\"nuoc_l\":%.1f}", Vw);
        break;
    }
    default:
        printf("Chua ho tro SETCFG cho loai nay.\n");
        return;
    }

    if (ctx->ops.setcfg(ctx->ops.user_data, id, d->token, json) != 0) {
        printf("Cap nhat cau hinh that bai.\n");
        return;
    }
    printf("Cap nhat cau hinh thanh cong.\n");
}

/* Quan ly chuong: them chuong, dong bo thiet bi vao chuong */
/** @brief Menu con: quản lý chuồng (thêm chuồng, xem danh sách, đăng ký thiết bị). */
static void menu_manage_coop(struct UiContext *ctx) {
    if (!ctx) return;
    int back = 0;
    while (!back) {
        refresh_coops(ctx);
        printf("=== Quan ly chuong ===\n");
        printf("1. Them chuong\n");
        printf("2. Xem danh sach chuong\n");
        printf("3. Dang ky thiet bi moi\n");
        printf("0. Quay lai\n");
        printf("Chon: ");
        char line[8];
        if (read_line(line, sizeof(line)) != 0) break;
        int choice = atoi(line);
        switch (choice) {
        case 1: {
            char name[MAX_COOP_NAME];
            printf("Nhap ten chuong: ");
            if (read_line(name, sizeof(name)) != 0) break;
            if (name[0] == '\0' || strcmp(name, "0") == 0) {
                printf("Da huy them chuong.\n");
                break;
            }
            int new_id = 0;
            if (!ctx->ops.coop_add || ctx->ops.coop_add(ctx->ops.user_data, name, &new_id) != 0) {
                printf("Khong the them chuong.\n");
                break;
            }
            printf("Da them chuong [%d] '%s'.\n", new_id, name);
            break;
        }
        case 2:
            print_coops_with_connection(ctx);
            break;
        case 3: {
            if (!ctx->ops.add_device) {
                printf("Client chua ho tro dang ky thiet bi.\n");
                break;
            }
            size_t idx = 0;
            if (select_coop_index(&ctx->coop_list, &idx) != 0) break;
            const struct Coop *c = &ctx->coop_list.coops[idx];
            if (c->id <= 0) {
                printf("Chuong khong hop le.\n");
                break;
            }

            char dev_id[MAX_ID_LEN];
            char type_str[MAX_TYPE_LEN];
            char pw[MAX_PASSWORD_LEN];

            printf("Nhap ID thiet bi (vd FAN2): ");
            if (read_line(dev_id, sizeof(dev_id)) != 0 || dev_id[0] == '\0') {
                printf("ID khong hop le.\n");
                break;
            }
            printf("Nhap loai thiet bi (sensor/fan/heater/sprayer/feeder/drinker/egg_counter): ");
            if (read_line(type_str, sizeof(type_str)) != 0) break;
            enum DeviceType type = device_type_from_string(type_str);
            if (type == DEVICE_UNKNOWN) {
                printf("Loai thiet bi khong hop le.\n");
                break;
            }
            printf("Nhap mat khau (de trong = 123456): ");
            if (read_line(pw, sizeof(pw)) != 0) break;
            if (pw[0] == '\0') {
                strncpy(pw, "123456", sizeof(pw) - 1);
                pw[sizeof(pw) - 1] = '\0';
            }

            if (ctx->ops.add_device(ctx->ops.user_data, dev_id, type, pw, c->id) != 0) {
                printf("Dang ky that bai (co the trung ID hoac chuong khong ton tai).\n");
                break;
            }
            refresh_coops(ctx);
            printf("Da dang ky %s (%s) vao chuong %s.\n", dev_id, device_type_to_string(type), c->name);
            break;
        }
        case 0:
            back = 1;
            break;
        default:
            printf("Lua chon khong hop le.\n");
            break;
        }
    }
}

void ui_context_init(struct UiContext *ctx, const struct UiBackendOps *ops) {
    if (!ctx || !ops) {
        return;
    }
    memset(ctx, 0, sizeof(*ctx));
    ctx->ops = *ops;
    coop_list_init(&ctx->coop_list);
}

/* Vong lap menu chinh */
void ui_run(struct UiContext *ctx) {
    if (!ctx) {
        return;
    }
    int running = 1;
    while (running) {
        printf("===== MENU =====\n");
        printf("1. Ket noi thiet bi\n");
        printf("2. Xem thong tin thiet bi\n");
        printf("3. Bat/tat thiet bi\n");
        printf("4. Thiet lap tham so\n");
        printf("5. Dieu khien truc tiep\n");
        printf("6. Quan ly chuong nuoi\n");
        printf("0. Thoat\n");
        printf("Chon: ");

        char line[8];
        if (read_line(line, sizeof(line)) != 0) {
            break;
        }
        int choice = atoi(line);

        switch (choice) {
        case 1:
            menu_connect(ctx);
            break;
        case 2:
            menu_info(ctx);
            break;
        case 3:
            menu_control(ctx, 0);
            break;
        case 4:
            menu_setcfg(ctx);
            break;
        case 5:
            menu_control(ctx, 1);
            break;
        case 6:
            menu_manage_coop(ctx);
            break;
        case 0:
            running = 0;
            break;
        default:
            printf("Lua chon khong hop le.\n");
            break;
        }
    }
    printf("Tam biet!\n");
}
