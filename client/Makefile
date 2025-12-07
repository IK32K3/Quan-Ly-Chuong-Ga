CC ?= gcc
CFLAGS ?= -Wall -Wextra -std=c99
INCLUDES = -I. -I.. -I../shared -I../server

SRCS = main_client.c ui_client.c coop_client.c \
	../server/devices.c \
	../shared/types.c

BIN = demo_client

all: $(BIN)

$(BIN): $(SRCS)
	$(CC) $(CFLAGS) $(INCLUDES) -o $@ $(SRCS)

clean:
	rm -f $(BIN)

.PHONY: all clean
