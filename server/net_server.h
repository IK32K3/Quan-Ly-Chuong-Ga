#ifndef NET_SERVER_H
#define NET_SERVER_H

#include <sys/socket.h>
#include <netinet/in.h>
#include <poll.h>
#include <stddef.h>
#include "../shared/config.h"
#include "../shared/protocol.h"

/**
 * @brief Thông tin kết nối của một client đang được server quản lý.
 */
struct ClientConnection {
    int fd;  // File descriptor socket
    char buffer[MAX_LINE_LEN];  // Buffer cho dòng nhận
    size_t buf_pos;  // Vị trí trong buffer
};

/**
 * @brief Tạo socket server, bind và listen.
 * @return FD socket server (>=0) nếu thành công, -1 nếu lỗi.
 */
int server_init(int port, int backlog);

/**
 * @brief Chạy vòng lặp chính của server (accept/poll/đọc dòng).
 */
void server_run(int server_fd);

/**
 * @brief Gửi một dòng (không gồm `\n`) tới client và tự thêm newline.
 * @return 0 nếu gửi thành công, -1 nếu lỗi.
 */
int send_line(int fd, const char *line);

/**
 * @brief Đọc dữ liệu từ client, tách theo newline và xử lý từng dòng.
 */
void handle_client_line(struct ClientConnection *conn, struct pollfd *pfd);

#endif  /* NET_SERVER_H */
