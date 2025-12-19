#ifndef SHARED_PROTOCOL_H
#define SHARED_PROTOCOL_H

#include "types.h"

/** @brief Các command client có thể gửi lên server. */
enum CommandType {
    CMD_SCAN = 0,
    CMD_CONNECT,
    CMD_INFO,
    CMD_CONTROL,
    CMD_SETCFG,
    CMD_CHPASS,
    CMD_BYE,
    CMD_ADD_DEVICE,
    CMD_ASSIGN_DEVICE,
    CMD_COOP_LIST,
    CMD_COOP_ADD,
    CMD_UNKNOWN
};

/** @brief Mã response server trả về (theo nhóm). */
enum ResponseCode {
    // Success (1xx-2xx)
    RESP_READY = 100,
    RESP_DEVICE = 110,
    RESP_NO_DEVICE_SCAN = 111,
    RESP_CONNECT_OK = 120,
    RESP_INFO_OK = 130,
    RESP_CONTROL_OK = 140,
    RESP_SETCFG_OK = 150,
    RESP_PASS_OK = 160,
    RESP_BYE_OK = 170,
    RESP_ADD_OK = 180,
    RESP_ASSIGN_OK = 181,
    RESP_COOP = 190,
    RESP_COOPADD_OK = 191,
    RESP_NO_COOP = 192,
    
    // Client errors (2xx-3xx)
    RESP_WRONG_PASSWORD = 221,
    RESP_NO_DEVICE = 222,
    RESP_NOT_CONNECTED = 331,
    
    // Request errors (4xx)
    RESP_BAD_REQUEST = 400
};

/**
 * @brief Parse command word (vd "SCAN", "CONNECT", ...) thành `CommandType`.
 * Không phân biệt hoa/thường. Trả `CMD_UNKNOWN` nếu không khớp.
 */
enum CommandType protocol_command_from_string(const char *word);

/** @brief Chuyển `CommandType` sang chuỗi chuẩn dùng để debug/log. */
const char *protocol_command_to_string(enum CommandType cmd);

/**
 * @brief Format một dòng response theo mẫu: "<code> <text> [payload]".
 * @return 0 nếu format thành công, -1 nếu lỗi/buffer không đủ.
 */
int protocol_format_line(char *out, size_t len, int code, const char *text, const char *payload);

/** @brief Response chào ban đầu khi client vừa kết nối. */
int protocol_format_ready(char *out, size_t len);

/** @brief Response cho một thiết bị (không kèm coop_id). */
int protocol_format_device(char *out, size_t len, const char *id, enum DeviceType type);

/** @brief Response cho một thiết bị (kèm coop_id). */
int protocol_format_device_ex(char *out, size_t len, const char *id, enum DeviceType type, int coop_id);

/** @brief Response khi SCAN không tìm thấy thiết bị. */
int protocol_format_no_device_scan(char *out, size_t len);

/** @brief Response CONNECT thành công (payload = token). */
int protocol_format_connect_ok(char *out, size_t len, const char *token);

/** @brief Response sai mật khẩu khi CONNECT/CHPASS. */
int protocol_format_wrong_password(char *out, size_t len);

/** @brief Response khi không tìm thấy thiết bị theo ID. */
int protocol_format_no_device_err(char *out, size_t len);

/** @brief Response INFO thành công (payload = JSON). */
int protocol_format_info_ok(char *out, size_t len, const char *json);

/** @brief Response CONTROL thành công. */
int protocol_format_control_ok(char *out, size_t len);

/** @brief Response SETCFG thành công (payload = JSON mới). */
int protocol_format_setcfg_ok(char *out, size_t len, const char *json);

/** @brief Response đổi mật khẩu thành công. */
int protocol_format_pass_ok(char *out, size_t len);

/** @brief Response BYE thành công. */
int protocol_format_bye_ok(char *out, size_t len);

/** @brief Response ADD device thành công. */
int protocol_format_add_ok(char *out, size_t len);

/** @brief Response ASSIGN device->coop thành công. */
int protocol_format_assign_ok(char *out, size_t len);

/** @brief Response trả về 1 dòng chuồng: "COOP <id> <name>". */
int protocol_format_coop(char *out, size_t len, int coop_id, const char *name);

/** @brief Response COOPADD thành công (payload = coop_id). */
int protocol_format_coopadd_ok(char *out, size_t len, int coop_id);

/** @brief Response khi chưa có chuồng nào. */
int protocol_format_no_coop(char *out, size_t len);

/** @brief Response khi token chưa hợp lệ hoặc chưa CONNECT. */
int protocol_format_not_connected(char *out, size_t len);

/** @brief Response khi request sai format/thiếu tham số. */
int protocol_format_bad_request(char *out, size_t len);

#endif  /* SHARED_PROTOCOL_H */
