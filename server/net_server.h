#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include "config.h"
#include "protocol.h"

// Struct cho client kết nối
struct ClientConnection {
    int fd;  // File descriptor socket
    char buffer[MAX_LINE_LEN];  // Buffer cho dòng nhận
    size_t buf_pos;  // Vị trí trong buffer
};

// Hàm khởi tạo server
int server_init(int port, int backlog);

// Hàm chạy server (vòng lặp chính: accept, poll, xử lý dòng)
void server_run(int server_fd);

// Hàm gửi dòng response đến client
int send_line(int fd, const char *line);

// Hàm nhận và xử lý dòng từ client (gọi handle_command từ B)
void handle_client_line(struct ClientConnection *conn);

#endif  /* NET_SERVER_H */