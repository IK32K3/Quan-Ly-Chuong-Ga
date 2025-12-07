#ifndef NET_CLIENT_H
#define NET_CLIENT_H

#include <sys/socket.h>
#include "config.h"

// Hàm kết nối đến server
int client_connect(const char *host, int port);

// Hàm gửi dòng lệnh
int client_send_line(int fd, const char *line);

// Hàm nhận dòng response
int client_recv_line(int fd, char *buffer, size_t len);

// Hàm đóng kết nối
void client_disconnect(int fd);

#endif  /* NET_CLIENT_H */