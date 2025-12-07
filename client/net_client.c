#include "net_client.h"
#include <unistd.h>
#include <string.h>
#include <stdio.h>
#include <netinet/in.h>
#include <arpa/inet.h>

int client_connect(const char *host, int port) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) return -1;

    struct sockaddr_in addr = {0};
    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    if (inet_pton(AF_INET, host, &addr.sin_addr) <= 0) {
        close(fd);
        return -1;
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        close(fd);
        return -1;
    }

    return fd;
}

int client_send_line(int fd, const char *line) {
    size_t len = strlen(line);
    if (write(fd, line, len) != (ssize_t)len) return -1;
    return write(fd, "\n", 1) != 1 ? -1 : 0;
}

int client_recv_line(int fd, char *buffer, size_t len) {
    ssize_t n = read(fd, buffer, len - 1);
    if (n <= 0) return -1;
    buffer[n] = '\0';
    // Loại bỏ \n nếu có
    char *end = strchr(buffer, '\n');
    if (end) *end = '\0';
    return 0;
}

void client_disconnect(int fd) {
    close(fd);
}