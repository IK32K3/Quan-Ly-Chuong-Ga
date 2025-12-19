#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <sys/socket.h>
#include "../shared/config.h"

/**
 * @brief Kết nối TCP tới server.
 * @return FD socket nếu thành công, -1 nếu lỗi.
 */
int client_connect(const char *host, int port);

/**
 * @brief Gửi một dòng lệnh (không gồm `\n`) và tự thêm newline.
 * @return 0 nếu thành công, -1 nếu lỗi.
 */
int client_send_line(int fd, const char *line);

/**
 * @brief Nhận một dòng response (blocking, đọc tới `\n`).
 * @return 0 nếu nhận được 1 dòng, -1 nếu lỗi/đóng kết nối.
 */
int client_recv_line(int fd, char *buffer, size_t len);

/**
 * @brief Nhận một dòng response với timeout (ms).
 * @return 0 nếu có dữ liệu và đọc thành công, 1 nếu timeout, -1 nếu lỗi/đóng.
 */
int client_recv_line_timeout(int fd, char *buffer, size_t len, int timeout_ms);

/** @brief Đóng socket client. */
void client_disconnect(int fd);

#endif  /* NET_CLIENT_H */
