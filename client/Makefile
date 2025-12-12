CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99
INCLUDES = -I. -I.. -I../shared

SRCS = main_client.c ui_client.c coop_client.c net_client.c \
	../shared/types.c ../shared/protocol.c

BIN = client_app

all: $(BIN)

$(BIN): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRCS)

clean:
	rm -f $(BIN)

.PHONY: all clean
