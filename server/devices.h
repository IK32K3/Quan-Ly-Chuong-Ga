#ifndef SERVER_DEVICES_H
#define SERVER_DEVICES_H

#include <stddef.h>
#include "../shared/config.h"
#include "../shared/types.h"

/** @brief Trạng thái bật/tắt đơn giản cho các thiết bị có điều khiển nguồn. */
enum DevicePowerState {
    DEVICE_OFF = 0,
    DEVICE_ON = 1
};

/** @brief Một mốc lịch (schedule) theo giờ trong ngày cho feeder/drinker. */
struct ScheduleEntry {
    char time[6];   /* HH:MM */
    double food;    /* kg */
    double water;   /* L */
};

/** @brief Dữ liệu cho cảm biến nhiệt độ/độ ẩm. */
struct SensorData {
    double temperature;
    double humidity;
    char unit_temperature[4];
    char unit_humidity[4];
};

/** @brief Dữ liệu cho bộ đếm trứng. */
struct EggCounterData {
    int egg_count;
};

/** @brief Dữ liệu/cấu hình cho máy cho ăn. */
struct FeederData {
    enum DevicePowerState state;
    double W;   /* kg per meal */
    double Vw;  /* L per meal */
    char unit_food[8];
    char unit_water[8];
    struct ScheduleEntry schedule[MAX_SCHEDULE_ENTRIES];
    size_t schedule_count;
};

/** @brief Dữ liệu/cấu hình cho máy cho uống. */
struct DrinkerData {
    enum DevicePowerState state;
    double Vw;  /* L per meal */
    char unit_water[8];
    struct ScheduleEntry schedule[MAX_SCHEDULE_ENTRIES];
    size_t schedule_count;
};

/** @brief Dữ liệu/cấu hình cho quạt. */
struct FanData {
    enum DevicePowerState state;
    double Tmax;
    double Tp1;
    char unit_temp[4];
};

/** @brief Dữ liệu/cấu hình cho đèn sưởi. */
struct HeaterData {
    enum DevicePowerState state;
    double Tmin;
    double Tp2;
    char mode[8];       /* e.g., AUTO/MANUAL */
    char unit_temp[4];
};

/** @brief Dữ liệu/cấu hình cho máy phun sương. */
struct SprayerData {
    enum DevicePowerState state;
    double Hmin;
    double Hp;
    double Vh;
    char unit_humidity[4];
    char unit_flow[8];  /* L/h */
};

/** @brief Union dữ liệu theo từng loại thiết bị. */
union DeviceData {
    struct SensorData sensor;
    struct EggCounterData egg_counter;
    struct FeederData feeder;
    struct DrinkerData drinker;
    struct FanData fan;
    struct HeaterData heater;
    struct SprayerData sprayer;
};

/** @brief Thiết bị trong hệ thống (định danh + mật khẩu + dữ liệu theo type). */
struct Device {
    struct DeviceIdentity identity;
    char password[MAX_PASSWORD_LEN];
    union DeviceData data;
};

/** @brief Context quản lý danh sách thiết bị trên server. */
struct DevicesContext {
    struct Device devices[MAX_DEVICES];
    size_t count;
};

/**
 * @brief Khởi tạo context thiết bị với dữ liệu mặc định.
 */
void devices_context_init(struct DevicesContext *ctx);

/**
 * @brief Quét danh sách thiết bị hiện có và copy ra mảng `DeviceIdentity`.
 * @return Số phần tử đã ghi vào `out`.
 */
size_t devices_scan(const struct DevicesContext *ctx, struct DeviceIdentity *out, size_t max_out);

/**
 * @brief Tìm thiết bị theo ID (bản mutable).
 * @return Con trỏ tới thiết bị nếu tìm thấy, NULL nếu không có.
 */
struct Device *devices_find(struct DevicesContext *ctx, const char *id);

/**
 * @brief Tìm thiết bị theo ID (bản const).
 * @return Con trỏ tới thiết bị nếu tìm thấy, NULL nếu không có.
 */
const struct Device *devices_find_const(const struct DevicesContext *ctx, const char *id);

/**
 * @brief Đổi mật khẩu thiết bị.
 * @return 0 nếu thành công, -2 nếu sai mật khẩu cũ, giá trị âm khác nếu lỗi.
 */
int devices_change_password(struct Device *dev, const char *old_pw, const char *new_pw);

/**
 * @brief Bật/tắt thiết bị (nếu type hỗ trợ).
 * @return 0 nếu thành công, giá trị âm nếu không hỗ trợ hoặc tham số không hợp lệ.
 */
int devices_set_state(struct Device *dev, enum DevicePowerState state);

/** @brief Cho ăn ngay (chỉ áp dụng cho `DEVICE_FEEDER`). */
int devices_feed_now(struct Device *dev, double food, double water);

/** @brief Cho uống ngay (chỉ áp dụng cho `DEVICE_DRINKER`). */
int devices_drink_now(struct Device *dev, double water);

/** @brief Phun ngay (chỉ áp dụng cho `DEVICE_SPRAYER`). */
int devices_spray_now(struct Device *dev, double Vh);

/** @brief Cập nhật cấu hình quạt (ngưỡng bật/tắt). */
int devices_set_config_fan(struct Device *dev, double Tmax, double Tp1);

/** @brief Cập nhật cấu hình đèn sưởi (ngưỡng bật/tắt + mode). */
int devices_set_config_heater(struct Device *dev, double Tmin, double Tp2, const char *mode);

/** @brief Cập nhật cấu hình phun sương (ngưỡng độ ẩm + lưu lượng). */
int devices_set_config_sprayer(struct Device *dev, double Hmin, double Hp, double Vh);

/** @brief Cập nhật cấu hình feeder (khẩu phần + lịch). */
int devices_set_config_feeder(struct Device *dev, double W, double Vw, const struct ScheduleEntry *schedule, size_t schedule_count);

/** @brief Cập nhật cấu hình drinker (lượng nước + lịch). */
int devices_set_config_drinker(struct Device *dev, double Vw, const struct ScheduleEntry *schedule, size_t schedule_count);

/**
 * @brief Xuất thông tin thiết bị ra JSON (1 object).
 * @return 0 nếu thành công, -1 nếu lỗi (buffer không đủ hoặc type không hợp lệ).
 */
int devices_info_json(const struct Device *dev, char *out_json, size_t out_len);

/* Them thiet bi moi voi cau hinh mac dinh theo type */
/**
 * @brief Thêm thiết bị mới vào context (kèm mật khẩu và coop_id).
 * @return 0 nếu thành công, giá trị âm nếu lỗi (đầy, trùng ID, tham số sai,...).
 */
int devices_add(struct DevicesContext *ctx, const char *id, enum DeviceType type, const char *password, int coop_id);

/* Tao thiet bi voi thong so mac dinh theo type (phuc vu load file scan/devices). */
/**
 * @brief Khởi tạo struct `Device` với thông số mặc định theo `type`.
 */
void devices_init_default_device(struct Device *dev, enum DeviceType type, const char *id, const char *password);

#endif /* SERVER_DEVICES_H */
