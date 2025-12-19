#include "net_server.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <poll.h>

// Giả định handle_command từ B trả về char* (response) hoặc NULL
extern char* handle_command(int fd, enum CommandType cmd, char *args);

/**
 * @file net_server.c
 * @brief Vòng lặp network của server: accept, poll, đọc dòng và dispatch command.
 */

/** @see server_init() */
int server_init(int port, int backlog) {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) {
        perror("socket");
        return -1;
    }

    int yes = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes)) < 0) {
        perror("setsockopt(SO_REUSEADDR)");
        close(server_fd);
        return -1;
    }

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = INADDR_ANY;
    addr.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, backlog) < 0) {
        perror("listen");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

/** @see server_run() */
void server_run(int server_fd) {
    /* fds[0] = server_fd, fds[i+1] <-> clients[i] */
    struct pollfd fds[MAX_DEVICES + 1];
    struct ClientConnection clients[MAX_DEVICES];
    int nfds = MAX_DEVICES + 1;

    fds[0].fd = server_fd;
    fds[0].events = POLLIN;

    for (int i = 0; i < MAX_DEVICES; ++i) {
        clients[i].fd = -1;
        clients[i].buf_pos = 0;
        fds[i + 1].fd = -1;          /* poll bo qua fd am */
        fds[i + 1].events = POLLIN;  /* dung chung */
    }

    while (1) {
        int ret = poll(fds, nfds, -1);
        if (ret < 0) {
            perror("poll");
            break;
        }

        // Xử lý server_fd: accept client mới
        if (fds[0].revents & POLLIN) {
            struct sockaddr_in client_addr;
            socklen_t addr_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &addr_len);
            if (client_fd >= 0) {
                // Tìm slot trống cho client
                for (int i = 0; i < MAX_DEVICES; ++i) {
                    if (clients[i].fd == -1) {
                        clients[i].fd = client_fd;
                        clients[i].buf_pos = 0;
                        fds[i + 1].fd = client_fd;
                        fds[i + 1].events = POLLIN;
                        // Gửi READY
                        char ready_line[MAX_LINE_LEN];
                        protocol_format_ready(ready_line, sizeof(ready_line));
                        send_line(client_fd, ready_line);
                        break;
                    }
                }
            }
        }

        // Xử lý client fds (fds[i+1] <-> clients[i])
        for (int i = 0; i < MAX_DEVICES; ++i) {
            if (fds[i + 1].fd < 0) {
                continue;
            }
            if (fds[i + 1].revents & (POLLIN | POLLHUP | POLLERR)) {
                handle_client_line(&clients[i], &fds[i + 1]);
            }
        }
    }
}

/** @see send_line() */
int send_line(int fd, const char *line) {
    size_t len = strlen(line);
    if (write(fd, line, len) != (ssize_t)len) {
        return -1;
    }
    return write(fd, "\n", 1) != 1 ? -1 : 0;
}

/** @brief Tách dòng input thành `cmd` và `args` (sửa in-place bằng cách chèn `\0`). */
static void parse_cmd_line(char *line, char **cmd_out, char **args_out) {
    *cmd_out = NULL;
    *args_out = NULL;
    if (!line) return;
    while (*line == ' ') line++;
    if (*line == '\0') return;
    char *space = strchr(line, ' ');
    if (!space) {
        *cmd_out = line;
        return;
    }
    *space = '\0';
    *cmd_out = line;
    char *args = space + 1;
    while (*args == ' ') args++;
    *args_out = (*args == '\0') ? NULL : args;
}

/** @see handle_client_line() */
void handle_client_line(struct ClientConnection *conn, struct pollfd *pfd) {
    char *line_end;
    ssize_t n = read(conn->fd, conn->buffer + conn->buf_pos, MAX_LINE_LEN - conn->buf_pos - 1);
    if (n <= 0) {
        // Client disconnect
        close(conn->fd);
        conn->fd = -1;
        if (pfd) {
            pfd->fd = -1;
            pfd->revents = 0;
        }
        return;
    }
    conn->buf_pos += n;
    conn->buffer[conn->buf_pos] = '\0';

    while ((line_end = strchr(conn->buffer, '\n'))) {
        *line_end = '\0';
        // Parse lệnh (format: CMD [args...])
        char *cmd_str = NULL;
        char *args = NULL;
        parse_cmd_line(conn->buffer, &cmd_str, &args);
        enum CommandType cmd = protocol_command_from_string(cmd_str);
        if (cmd != CMD_UNKNOWN) {
            // Gọi handle_command từ B và gửi response
            char *response = handle_command(conn->fd, cmd, args);
            if (response) {
                send_line(conn->fd, response);
                free(response);  // Giả định B allocate với malloc
            } else if (cmd != CMD_SCAN && cmd != CMD_COOP_LIST) {
                char bad_req[MAX_LINE_LEN];
                protocol_format_bad_request(bad_req, sizeof(bad_req));
                send_line(conn->fd, bad_req);
            }
        } else {
            char bad_req[MAX_LINE_LEN];
            protocol_format_bad_request(bad_req, sizeof(bad_req));
            send_line(conn->fd, bad_req);
        }
        // Shift buffer (sửa lỗi tính toán)
        size_t shift_len = conn->buf_pos - (line_end - conn->buffer) - 1;
        memmove(conn->buffer, line_end + 1, shift_len);
        conn->buf_pos = shift_len;
    }
}
