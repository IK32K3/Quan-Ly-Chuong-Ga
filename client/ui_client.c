#include "ui_client.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* Helpers cho UI: tra cuu, nhap lieu va parse JSON don gian */

static struct ClientDevice *find_client_device(struct UiContext *ctx, const char *id) {
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

static int select_coop_index(const struct CoopList *list, size_t *out_index) {
    if (list->count == 0) {
        printf("Chua co chuong nao. Hay them chuong truoc.\n");
        return -1;
    }
    coop_print(list);
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

static int select_device_in_coop(const struct UiContext *ctx, size_t coop_index, char *out_id, size_t out_id_len, enum DeviceType *out_type) {
    if (!ctx || coop_index >= ctx->coop_list.count) return -1;
    const struct Coop *c = &ctx->coop_list.coops[coop_index];
    if (c->device_count == 0) {
        printf("Chua co thiet bi nao trong chuong nay. Hay them truoc.\n");
        return -1;
    }
    printf("Thiet bi trong chuong %s:\n", c->name);
    for (size_t i = 0; i < c->device_count; ++i) {
        printf("%zu) %s (%s)\n", i + 1, c->devices[i].device_id, device_type_to_string(c->devices[i].type));
    }
    printf("Chon thiet bi (0 de quay lai): ");
    char line[16];
    if (read_line(line, sizeof(line)) != 0) return -1;
    long idx = strtol(line, NULL, 10);
    if (idx == 0) {
        return -1;
    }
    if (idx < 0 || (size_t)idx > c->device_count) {
        printf("Lua chon khong hop le.\n");
        return -1;
    }
    const struct CoopDevice *d = &c->devices[idx - 1];
    strncpy(out_id, d->device_id, out_id_len - 1);
    out_id[out_id_len - 1] = '\0';
    if (out_type) *out_type = d->type;
    return 0;
}

/* Dong bo tat ca thiet bi tu backend vao chuong (mo phong chuong co day thiet bi) */
static void autopopulate_coop_devices(struct UiContext *ctx, size_t coop_index) {
    if (!ctx || coop_index >= ctx->coop_list.count) return;
    struct DeviceIdentity devices[MAX_DEVICES];
    size_t found = 0;
    if (ctx->ops.scan(ctx->ops.user_data, devices, MAX_DEVICES, &found) != 0 || found == 0) {
        return;
    }
    for (size_t i = 0; i < found; ++i) {
        coop_add_device(&ctx->coop_list, coop_index, devices[i].id, devices[i].type);
    }
}

static const char *find_key(const char *json, const char *key) {
    if (!json || !key) return NULL;
    char pattern[64];
    snprintf(pattern, sizeof(pattern), "\"%s\"", key);
    return strstr(json, pattern);
}

static int json_get_string(const char *json, const char *key, char *out, size_t out_len) {
    const char *p = find_key(json, key);
    if (!p) return -1;
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '"') return -1;
    p++;
    size_t idx = 0;
    while (*p && *p != '"' && idx + 1 < out_len) {
        out[idx++] = *p++;
    }
    out[idx] = '\0';
    return 0;
}

static int json_get_double(const char *json, const char *key, double *out) {
    const char *p = find_key(json, key);
    if (!p) return -1;
    p = strchr(p, ':');
    if (!p) return -1;
    p++;
    while (*p == ' ' || *p == '\t') p++;
    if (sscanf(p, "%lf", out) == 1) {
        return 0;
    }
    return -1;
}

/* Hien thi thong tin thiet bi theo tung type*/
static void display_info(const char *json) {
    /* Thu parse thong tin chi tiet theo type */
    char type[32];
    char id[MAX_ID_LEN];
    if (json_get_string(json, "type", type, sizeof(type)) != 0 ||
        json_get_string(json, "device_id", id, sizeof(id)) != 0) {
        printf("Khong doc duoc thong tin thiet bi.\n");
        return;
    }

    printf("Thiet bi: %s (%s)\n", id, type);
    if (strcmp(type, "sensor") == 0) {
        double t = 0.0, h = 0.0;
        char ut[8], uh[8];
        json_get_double(json, "temperature", &t);
        json_get_double(json, "humidity", &h);
        json_get_string(json, "unit_temperature", ut, sizeof(ut));
        json_get_string(json, "unit_humidity", uh, sizeof(uh));
        printf("Nhiet do: %.1f %s\n", t, ut);
        printf("Do am: %.1f %s\n", h, uh);
    } else if (strcmp(type, "fan") == 0) {
        char state[8], unit[8];
        double Tmax = 0.0, Tp1 = 0.0;
        json_get_string(json, "state", state, sizeof(state));
        json_get_double(json, "Tmax", &Tmax);
        json_get_double(json, "Tp1", &Tp1);
        json_get_string(json, "unit_temp", unit, sizeof(unit));
        printf("Trang thai: %s\n", state);
        printf("Tmax: %.1f %s | Tp1: %.1f %s\n", Tmax, unit, Tp1, unit);
    } else if (strcmp(type, "heater") == 0) {
        char state[8], unit[8], mode[16];
        double Tmin = 0.0, Tp2 = 0.0;
        json_get_string(json, "state", state, sizeof(state));
        json_get_double(json, "Tmin", &Tmin);
        json_get_double(json, "Tp2", &Tp2);
        json_get_string(json, "mode", mode, sizeof(mode));
        json_get_string(json, "unit_temp", unit, sizeof(unit));
        printf("Trang thai: %s (mode %s)\n", state, mode);
        printf("Tmin: %.1f %s | Tp2: %.1f %s\n", Tmin, unit, Tp2, unit);
    } else if (strcmp(type, "sprayer") == 0) {
        char state[8], uh[8], uf[8];
        double Hmin = 0.0, Hp = 0.0, Vh = 0.0;
        json_get_string(json, "state", state, sizeof(state));
        json_get_double(json, "Hmin", &Hmin);
        json_get_double(json, "Hp", &Hp);
        json_get_double(json, "Vh", &Vh);
        json_get_string(json, "unit_humidity", uh, sizeof(uh));
        json_get_string(json, "unit_flow", uf, sizeof(uf));
        printf("Trang thai: %s\n", state);
        printf("Nguong Hmin/Hp: %.1f/%.1f %s\n", Hmin, Hp, uh);
        printf("Toc do phun: %.1f %s\n", Vh, uf);
    } else if (strcmp(type, "feeder") == 0) {
        char state[8], uf[8], uw[8];
        double W = 0.0, Vw = 0.0;
        json_get_string(json, "state", state, sizeof(state));
        json_get_double(json, "W", &W);
        json_get_double(json, "Vw", &Vw);
        json_get_string(json, "unit_food", uf, sizeof(uf));
        json_get_string(json, "unit_water", uw, sizeof(uw));
        printf("Trang thai: %s\n", state);
        printf("Suat an: W=%.1f %s, Nuoc=%.1f %s\n", W, uf, Vw, uw);
    } else if (strcmp(type, "drinker") == 0) {
        char state[8], uw[8];
        double Vw = 0.0;
        json_get_string(json, "state", state, sizeof(state));
        json_get_double(json, "Vw", &Vw);
        json_get_string(json, "unit_water", uw, sizeof(uw));
        printf("Trang thai: %s\n", state);
        printf("Luong nuoc moi lan: %.1f %s\n", Vw, uw);
    } else if (strcmp(type, "egg_counter") == 0) {
        int eggs = 0;
        if (json_get_double(json, "egg_count", (double *)&eggs) == 0) {
            printf("So trung dem duoc: %d\n", eggs);
        }
    } else {
        printf("Loai thiet bi chua ho tro hien thi chi tiet.\n");
    }
}

static void menu_connect(struct UiContext *ctx) {
    size_t coop_idx = 0;
    if (select_coop_index(&ctx->coop_list, &coop_idx) != 0) {
        return;
    }
    /* Tu dong quet & dong bo thiet bi trong chuong truoc khi ket noi */
    autopopulate_coop_devices(ctx, coop_idx);
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

static const struct ClientDevice *require_connected(struct UiContext *ctx, const char *id) {
    struct ClientDevice *d = find_client_device(ctx, id);
    if (!d || !d->connected) {
        printf("Chua ket noi %s. Hay ket noi truoc.\n", id);
        return NULL;
    }
    return d;
}

/* Menu xem thong tin: chi cho phep thiet bi da ket noi trong chuong */
static void menu_info(struct UiContext *ctx) {
    size_t coop_idx = 0;
    if (select_coop_index(&ctx->coop_list, &coop_idx) != 0) {
        return;
    }
    const struct Coop *c = &ctx->coop_list.coops[coop_idx];
    size_t mapped[MAX_DEVICES];
    size_t mapped_count = 0;
    for (size_t i = 0; i < c->device_count; ++i) {
        struct ClientDevice *cd = find_client_device(ctx, c->devices[i].device_id);
        if (cd && cd->connected) {
            mapped[mapped_count++] = i;
        }
    }
    if (mapped_count == 0) {
        printf("Chua ket noi thiet bi nao trong chuong nay.\n");
        return;
    }
    printf("Thiet bi da ket noi trong chuong %s:\n", c->name);
    for (size_t i = 0; i < mapped_count; ++i) {
        const struct CoopDevice *d = &c->devices[mapped[i]];
        printf("%zu) %s (%s)\n", i + 1, d->device_id, device_type_to_string(d->type));
    }
    printf("Chon thiet bi (0 de quay lai): ");
    char line[16];
    if (read_line(line, sizeof(line)) != 0) return;
    long pick = strtol(line, NULL, 10);
    if (pick == 0) return;
    if (pick < 0 || (size_t)pick > mapped_count) {
        printf("Lua chon khong hop le.\n");
        return;
    }
    const char *id = c->devices[mapped[pick - 1]].device_id;
    const struct ClientDevice *d = require_connected(ctx, id);
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
static void menu_control(struct UiContext *ctx, int direct_mode) {
    size_t coop_idx = 0;
    if (select_coop_index(&ctx->coop_list, &coop_idx) != 0) {
        return;
    }
    char id[MAX_ID_LEN];
    enum DeviceType type = DEVICE_UNKNOWN;
    if (select_device_in_coop(ctx, coop_idx, id, sizeof(id), &type) != 0) {
        return;
    }
    const struct ClientDevice *d = require_connected(ctx, id);
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
        snprintf(payload, sizeof(payload), "{\"food\":%.1f,\"water\":%.1f}", food, water);
    } else if (direct_mode && d->type == DEVICE_DRINKER) {
        double water = 0.0;
        if (read_double("Nhap luong nuoc (L): ", &water) != 0) return;
        strncpy(action, "DRINK_NOW", sizeof(action) - 1);
        snprintf(payload, sizeof(payload), "{\"water\":%.1f}", water);
    } else if (direct_mode && d->type == DEVICE_SPRAYER) {
        double Vh = 0.0;
        if (read_double("Nhap toc do phun Vh (L/h): ", &Vh) != 0) return;
        strncpy(action, "SPRAY_NOW", sizeof(action) - 1);
        snprintf(payload, sizeof(payload), "{\"Vh\":%.1f}", Vh);
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
static void menu_setcfg(struct UiContext *ctx) {
    size_t coop_idx = 0;
    if (select_coop_index(&ctx->coop_list, &coop_idx) != 0) {
        return;
    }
    char id[MAX_ID_LEN];
    enum DeviceType type = DEVICE_UNKNOWN;
    if (select_device_in_coop(ctx, coop_idx, id, sizeof(id), &type) != 0) {
        return;
    }
    const struct ClientDevice *d = require_connected(ctx, id);
    if (!d) {
        return;
    }

    char json[MAX_JSON_LEN] = {0};
    switch (d->type) {
    case DEVICE_FAN: {
        double Tmax, Tp1;
        if (read_double("Nhap nguong bat quat (Tmax, do C): ", &Tmax) != 0) return;
        if (read_double("Nhap nguong tat quat (Tp1, do C): ", &Tp1) != 0) return;
        snprintf(json, sizeof(json), "{\"Tmax\":%.1f,\"Tp1\":%.1f}", Tmax, Tp1);
        break;
    }
    case DEVICE_HEATER: {
        double Tmin, Tp2;
        if (read_double("Nhap nguong bat den suoi (Tmin, do C): ", &Tmin) != 0) return;
        if (read_double("Nhap nguong tat den suoi (Tp2, do C): ", &Tp2) != 0) return;
        snprintf(json, sizeof(json), "{\"Tmin\":%.1f,\"Tp2\":%.1f}", Tmin, Tp2);
        break;
    }
    case DEVICE_SPRAYER: {
        double Hmin, Hp, Vh;
        if (read_double("Nhap nguong do am bat phun (Hmin, %%): ", &Hmin) != 0) return;
        if (read_double("Nhap do am muc tieu (Hp, %%): ", &Hp) != 0) return;
        if (read_double("Nhap toc do phun (Vh, L/h): ", &Vh) != 0) return;
        snprintf(json, sizeof(json), "{\"Hmin\":%.1f,\"Hp\":%.1f,\"Vh\":%.1f}", Hmin, Hp, Vh);
        break;
    }
    case DEVICE_FEEDER: {
        double W, Vw;
        if (read_double("Nhap khau phan an moi lan (W, kg): ", &W) != 0) return;
        if (read_double("Nhap luong nuoc moi lan (Vw, L): ", &Vw) != 0) return;
        snprintf(json, sizeof(json), "{\"W\":%.1f,\"Vw\":%.1f}", W, Vw);
        break;
    }
    case DEVICE_DRINKER: {
        double Vw;
        if (read_double("Nhap luong nuoc moi lan (Vw, L): ", &Vw) != 0) return;
        snprintf(json, sizeof(json), "{\"Vw\":%.1f}", Vw);
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
static void menu_manage_coop(struct UiContext *ctx) {
    if (!ctx) return;
    int back = 0;
    while (!back) {
        printf("=== Quan ly chuong ===\n");
        printf("1. Them chuong\n");
        printf("2. Them thiet bi vao chuong\n");
        printf("3. Xem danh sach chuong\n");
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
            int idx = coop_add(&ctx->coop_list, name);
            if (idx < 0) {
                printf("Khong the them chuong (toi da %d).\n", MAX_COOPS);
            } else {
                printf("Da them chuong '%s'.\n", name);
                autopopulate_coop_devices(ctx, (size_t)idx);
            }
            break;
        }
        case 2: {
            size_t idx = 0;
            if (select_coop_index(&ctx->coop_list, &idx) != 0) break;
            autopopulate_coop_devices(ctx, idx);
            printf("Da dong bo toan bo thiet bi vao chuong %zu.\n", idx + 1);
            break;
        }
        case 3:
            coop_print(&ctx->coop_list);
            break;
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
