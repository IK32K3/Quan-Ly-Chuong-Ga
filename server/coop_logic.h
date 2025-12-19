#ifndef SERVER_COOP_LOGIC_H
#define SERVER_COOP_LOGIC_H

#include "../shared/protocol.h"

/**
 * @brief Khởi tạo lớp xử lý logic chuồng/trại phía server.
 */
void coop_logic_init(void);

/**
 * @brief Xử lý một lệnh (command) nhận từ client và tạo response.
 * @return Con trỏ `char*` được cấp phát bằng `malloc()` chứa 1 dòng response
 *         (không gồm ký tự xuống dòng). Caller phải `free()` sau khi gửi.
 *         Trả NULL cho các lệnh tự gửi nhiều dòng (vd `SCAN`, `COOPLIST`).
 */
char *handle_command(int fd, enum CommandType cmd, char *args);

#endif /* SERVER_COOP_LOGIC_H */
