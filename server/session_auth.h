#ifndef SESSION_AUTH_H
#define SESSION_AUTH_H
#include <stddef.h>

#include "../shared/config.h"

// Struct cho session
struct Session {
    char token[MAX_TOKEN_LEN];
    char device_id[MAX_ID_LEN];
    int active;  // 1 nếu active
};

// Hàm tạo token ngẫu nhiên
void generate_token(char *token, size_t len);

// Hàm tạo session mới cho device
int create_session(const char *device_id, char *token_out);

// Hàm kiểm tra token và lấy device_id
int validate_session(const char *token, char *device_id_out);

// Hàm kết thúc session
void end_session(const char *token);

#endif  /* SESSION_AUTH_H */
