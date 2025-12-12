#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <sys/socket.h>
#include "../shared/config.h"

// Hàm kết nối đến server
int client_connect(const char *host, int port);

// Hàm gửi dòng lệnh
int client_send_line(int fd, const char *line);

// Hàm nhận dòng response (blocking). Trả -1 nếu lỗi/đóng.
int client_recv_line(int fd, char *buffer, size_t len);

// Hàm nhận dòng response với timeout (ms). Trả 0 nếu có dữ liệu, 1 nếu timeout, -1 nếu lỗi/đóng.
int client_recv_line_timeout(int fd, char *buffer, size_t len, int timeout_ms);

// Hàm đóng kết nối
void client_disconnect(int fd);

#endif  /* NET_CLIENT_H */
