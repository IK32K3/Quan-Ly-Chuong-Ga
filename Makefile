CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99

CLIENT_BIN := client_app
SERVER_BIN := server_app

CLIENT_SRCS := \
	client/main_client.c \
	client/ui_client.c \
	client/coop_client.c \
	client/net_client.c \
	shared/types.c \
	shared/protocol.c

SERVER_SRCS := \
	server/main_server.c \
	server/net_server.c \
	server/coop_logic.c \
	server/coops.c \
	server/devices.c \
	server/session_auth.c \
	server/monitor_log.c \
	server/storage.c \
	shared/types.c \
	shared/protocol.c

.PHONY: all client server clean

all: client server

client: $(CLIENT_BIN)

server: $(SERVER_BIN)

$(CLIENT_BIN): $(CLIENT_SRCS)
	$(CC) $(CFLAGS) -Ishared -o $@ $(CLIENT_SRCS)

$(SERVER_BIN): $(SERVER_SRCS)
	$(CC) $(CFLAGS) -Ishared -o $@ $(SERVER_SRCS)

clean:
	rm -f $(CLIENT_BIN) $(SERVER_BIN)
	rm -rf bin
	rm -f client/client_app server/server_app
