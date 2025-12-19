#ifndef SESSION_AUTH_H
#define SESSION_AUTH_H
#include <stddef.h>

#include "../shared/config.h"

/** @brief Một session đăng nhập: token <-> device_id. */
struct Session {
    char token[MAX_TOKEN_LEN];
    char device_id[MAX_ID_LEN];
    int active;  // 1 nếu active
};

/**
 * @brief Sinh token ngẫu nhiên dạng chuỗi ASCII.
 */
void generate_token(char *token, size_t len);

/**
 * @brief Tạo session mới cho thiết bị.
 * @return 0 nếu tạo được session, -1 nếu không còn slot.
 */
int create_session(const char *device_id, char *token_out);

/**
 * @brief Kiểm tra token và trả về device_id tương ứng.
 * @return 0 nếu token hợp lệ, -1 nếu không tồn tại/không active.
 */
int validate_session(const char *token, char *device_id_out);

/**
 * @brief Kết thúc (invalidate) session theo token.
 */
void end_session(const char *token);

#endif  /* SESSION_AUTH_H */
