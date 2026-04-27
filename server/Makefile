CC      = gcc
CFLAGS  = -Wall -Wextra -g

TARGETS = server_tcp server_udp server_concurrent

all: $(TARGETS)

server_tcp: server_tcp.c
	$(CC) $(CFLAGS) -o $@ $^

server_udp: server_udp.c
	$(CC) $(CFLAGS) -o $@ $^

server_concurrent: server_concurrent.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS)

.PHONY: all clean
