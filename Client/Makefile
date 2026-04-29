CC     = gcc
CFLAGS = -Wall -Wextra -g

TARGETS = client_http client_tcp client_udp \
          client_service1 client_service2 client_service3

all: $(TARGETS)

client_http: client_http.c
	$(CC) $(CFLAGS) -o $@ $^

client_tcp: client_tcp.c
	$(CC) $(CFLAGS) -o $@ $^

client_udp: client_udp.c
	$(CC) $(CFLAGS) -o $@ $^

client_service1: client_service1.c
	$(CC) $(CFLAGS) -o $@ $^

client_service2: client_service2.c
	$(CC) $(CFLAGS) -o $@ $^

client_service3: client_service3.c
	$(CC) $(CFLAGS) -o $@ $^

clean:
	rm -f $(TARGETS) received_file.txt /tmp/out_service*.txt

.PHONY: all clean
