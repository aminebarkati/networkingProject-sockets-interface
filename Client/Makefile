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

TARGETS = client_http    \
          client_tcp      \
          client_udp      \
          client_service1 \
          client_service2 \
          client_service3

.PHONY: all clean test

all: $(TARGETS)
	@echo ""
	@echo "Compilation terminée. Binaires disponibles :"
	@for t in $(TARGETS); do echo "  ./$$t"; done
	@echo ""

client_http:    client_http.c
	$(CC) $(CFLAGS) -o $@ $<

client_tcp:     client_tcp.c
	$(CC) $(CFLAGS) -o $@ $<

client_udp:     client_udp.c
	$(CC) $(CFLAGS) -o $@ $<

client_service1: client_service1.c
	$(CC) $(CFLAGS) -o $@ $<

client_service2: client_service2.c
	$(CC) $(CFLAGS) -o $@ $<

client_service3: client_service3.c
	$(CC) $(CFLAGS) -o $@ $<

test: all
	@chmod +x test_parallele.sh
	@./test_parallele.sh

clean:
	rm -f $(TARGETS)
	rm -f fichier_recu_*.txt
	rm -rf logs_test/
	@echo "Nettoyage terminé."
