# =============================================================================
# Makefile — Personne B (Clients)
# =============================================================================
#
# Usage :
#   make              → compile tout
#   make client_http  → compile un seul binaire
#   make clean        → supprime les binaires et logs
#   make test         → lance le script de test parallèle
#

CC      = gcc
CFLAGS  = -Wall -Wextra -g    # -Wall : tous les warnings, -g : debug gdb
OUTPUT_DIR = output

BINARIES = client_http    \
           client_tcp      \
           client_udp      \
           client_service1 \
           client_service2 \
           client_service3

TARGETS = $(addprefix $(OUTPUT_DIR)/,$(BINARIES))

.PHONY: all clean test $(BINARIES)

all: $(TARGETS)
	@echo ""
	@echo "Compilation terminée. Binaires disponibles :"
	@for t in $(TARGETS); do echo "  ./$$t"; done
	@echo ""

$(OUTPUT_DIR):
	@mkdir -p $(OUTPUT_DIR)

$(OUTPUT_DIR)/%: %.c | $(OUTPUT_DIR)
	$(CC) $(CFLAGS) -o $@ $<

client_http:    $(OUTPUT_DIR)/client_http

client_tcp:     $(OUTPUT_DIR)/client_tcp

client_udp:     $(OUTPUT_DIR)/client_udp

client_service1: $(OUTPUT_DIR)/client_service1

client_service2: $(OUTPUT_DIR)/client_service2

client_service3: $(OUTPUT_DIR)/client_service3

test: all
	@chmod +x test_parallele.sh
	@cd $(OUTPUT_DIR) && ../test_parallele.sh

clean:
	rm -f $(TARGETS)
	rm -f $(OUTPUT_DIR)/fichier_recu_*.txt
	rm -rf $(OUTPUT_DIR)/logs_test/
	@echo "Nettoyage terminé."
