CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99
INCLUDES = -I. -I.. -I../shared

SRCS = main_server.c \
       coop_logic.c \
       devices.c \
       monitor_log.c \
       net_server.c \
       session_auth.c \
       storage.c \
       ../shared/types.c \
       ../shared/protocol.c

BIN = server_app

all: $(BIN)

$(BIN): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRCS)

clean:
	rm -f $(BIN)

.PHONY: all clean
