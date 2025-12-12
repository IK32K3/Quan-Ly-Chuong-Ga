#ifndef SERVER_COOP_LOGIC_H
#define SERVER_COOP_LOGIC_H

#include "../shared/protocol.h"

void coop_logic_init(void);
char *handle_command(int fd, enum CommandType cmd, char *args);

#endif /* SERVER_COOP_LOGIC_H */
