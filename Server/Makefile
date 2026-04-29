CC      = gcc
CFLAGS  = -Wall -Wextra -g
OUTPUT_DIR = output

BINARIES = server_tcp server_udp server_concurrent
TARGETS = $(addprefix $(OUTPUT_DIR)/,$(BINARIES))

all: $(TARGETS)

$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)

$(OUTPUT_DIR)/%: %.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -o $@ $<

server_tcp: $(OUTPUT_DIR)/server_tcp

server_udp: $(OUTPUT_DIR)/server_udp

server_concurrent: $(OUTPUT_DIR)/server_concurrent

clean:
	rm -f $(TARGETS)

.PHONY: all clean $(BINARIES)
