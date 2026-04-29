# TP Réseaux — Interface Sockets (GL2)

TP Client/Serveur en C utilisant l'interface sockets BSD pour des communications TCP et UDP.

## Structure

```
Client/          ← code côté client (Personne B)
server/          ← code côté serveur (Personne A)
```

## Démarrage rapide

```bash
# Compiler
cd server && make
cd Client && make

# Lancer le serveur TCP (phase 3)
cd server && ./server_tcp

# Lancer le client TCP dans un autre terminal
cd Client && ./client_tcp

# Lancer le serveur concurrent avec 3 services (phase 5)
cd server && ./server_concurrent

# Tester les 3 services en parallèle
cd Client && ./test_parallel.sh
```

## Documentation

Voir [server/ARCHITECTURE.md](server/ARCHITECTURE.md) pour les explications détaillées :
architecture, code annoté, chronogrammes des échanges, commandes de test.

## Phases du TP

| Phase | Fichiers | Description |
|-------|----------|-------------|
| 2 | `Client/client_http.c` | Client HTTP en mode connecté |
| 3 | `server/server_tcp.c` + `Client/client_tcp.c` | Transfert TCP, comptage de messages |
| 4 | `server/server_udp.c` + `Client/client_udp.c` | Transfert UDP, comparaison TCP/UDP |
| 5 | `server/server_concurrent.c` + `Client/client_service*.c` | Serveur concurrent, 3 services |