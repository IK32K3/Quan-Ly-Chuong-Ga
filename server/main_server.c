#include <stdio.h>
#include "net_server.h"
#include "coop_logic.h"
#include "../shared/config.h"

/** @brief Entry point của server: init dữ liệu và chạy vòng lặp network. */
int main(void) {
    coop_logic_init();

    int server_fd = server_init(DEFAULT_PORT, DEFAULT_BACKLOG);
    if (server_fd < 0) {
        fprintf(stderr, "Khong khoi tao duoc server\n");
        return 1;
    }

    printf("Server dang lang nghe tai cong %d\n", DEFAULT_PORT);
    server_run(server_fd);
    return 0;
}
