# TP Réseaux — Architecture & Explications

## Table des matières
1. [Vue d'ensemble du projet](#1-vue-densemble-du-projet)
2. [C'est quoi une socket ?](#2-cest-quoi-une-socket-)
3. [Les Makefiles expliqués](#3-les-makefiles-expliqués)
4. [Côté Serveur](#4-côté-serveur)
   - [server_tcp.c — Serveur TCP séquentiel](#41-server_tcpc--serveur-tcp-séquentiel)
   - [server_udp.c — Serveur UDP](#42-server_udpc--serveur-udp)
   - [server_concurrent.c — Serveur concurrent multi-services](#43-server_concurrentc--serveur-concurrent-multi-services)
5. [Côté Client](#5-côté-client)
   - [client_http.c — Client HTTP](#51-client_httpc--client-http)
   - [client_tcp.c — Client TCP](#52-client_tcpc--client-tcp)
   - [client_udp.c — Client UDP](#53-client_udpc--client-udp)
   - [client_service1/2/3.c — Clients de test](#54-client_service123c--clients-de-test)
6. [Flux complet des échanges](#6-flux-complet-des-échanges)
7. [Commandes pour tester](#7-commandes-pour-tester)

---

## 1. Vue d'ensemble du projet

```
networkingProject-sockets-interface/
│
├── Client/                       ← tout le code côté client
│   ├── client_http.c             Phase 2 : client HTTP (port 80)
│   ├── client_tcp.c              Phase 3 : client TCP (port 8080)
│   ├── client_udp.c              Phase 4 : client UDP (port 8080)
│   ├── client_service1.c         Phase 5 : test service heure (port 8080)
│   ├── client_service2.c         Phase 5 : test service processus (port 8081)
│   ├── client_service3.c         Phase 5 : test service fichier (port 8082)
│   ├── test_parallel.sh          Phase 5 : script de test de concurrence
│   ├── partie2.1-telnet_test     Phase 2 : capture telnet HEAD/GET
│   └── Makefile
│
└── server/                       ← tout le code côté serveur
    ├── server_tcp.c              Phase 3 : serveur TCP séquentiel
    ├── server_udp.c              Phase 4 : serveur UDP
    ├── server_concurrent.c       Phase 5 : serveur concurrent, 3 services
    ├── shared_file.txt           fichier servi par le service 3
    └── Makefile
```

### Qui fait quoi

| Phase | Serveur (Personne A) | Client (Personne B) |
|-------|---------------------|---------------------|
| 2 | — | `client_http.c` → port 80 |
| 3 | `server_tcp.c` → port 8080 | `client_tcp.c` → port 8080 |
| 4 | `server_udp.c` → port 8080 | `client_udp.c` → port 8080 |
| 5 | `server_concurrent.c` → 8080/8081/8082 | `client_service1/2/3.c` + `test_parallel.sh` |

---

## 2. C'est quoi une socket ?

Une **socket** est un point de communication entre deux programmes sur un réseau.
Imaginez un téléphone : une socket, c'est le combiné. Vous composez un numéro (adresse IP + port),
la ligne s'établit, et vous pouvez envoyer/recevoir des données.

En C, une socket est simplement un **entier** (`int`) que le système vous donne pour représenter
ce canal. On l'appelle **descripteur de fichier réseau** — exactement comme `fopen()` retourne un
`FILE*` pour un fichier, `socket()` retourne un `int` pour le réseau.

### TCP vs UDP

| | TCP | UDP |
|---|---|---|
| Connexion | Oui (comme un appel téléphonique) | Non (comme un SMS) |
| Ordre garanti | Oui | Non |
| Pertes possibles | Non (retransmission auto) | Oui |
| 1 send = 1 recv ? | **Non** — TCP fusionne les données | **Oui** — 1 datagramme = 1 message |
| Multi-clients facile ? | Nécessite fork/threads | Oui, adresse dans chaque paquet |
| Usage typique | HTTP, SSH, FTP | DNS, streaming, jeux |

---

## 3. Les Makefiles expliqués

### Structure commune

```makefile
CC     = gcc            # compilateur
CFLAGS = -Wall -Wextra -g  # options

# -Wall   : affiche tous les warnings
# -Wextra : encore plus de warnings
# -g      : inclut les infos de débogage (pour gdb)
```

### Règle de compilation (même logique dans les deux Makefiles)

```makefile
server_tcp: server_tcp.c
	$(CC) $(CFLAGS) -o $@ $^
#                   ^  ^   ^
#                   |  |   └── $^ = fichier(s) source (server_tcp.c)
#                   |  └────── $@ = nom de la cible  (server_tcp)
#                   └───────── -o  = nom de l'exécutable produit
```

### Commandes

```bash
# Dans server/
make              # compile les 3 serveurs
make server_tcp   # compile uniquement server_tcp
make clean        # supprime les exécutables

# Dans Client/
make              # compile les 6 clients
make client_http  # compile uniquement client_http
make clean        # supprime les exécutables + received_file.txt
```

---

## 4. Côté Serveur

### 4.1. server_tcp.c — Serveur TCP séquentiel

#### Les includes

```c
#include <sys/socket.h>   // socket(), bind(), listen(), accept(), send(), recv()
#include <netinet/in.h>   // struct sockaddr_in, htons()
#include <arpa/inet.h>    // inet_ntoa() — convertit IP binaire → texte lisible
#include <time.h>         // time(), strftime(), localtime()
#include <unistd.h>       // close(), sleep()
```

#### Création et configuration de la socket

```c
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
//                      ^        ^
//                      IPv4     TCP (flux continu)

int opt = 1;
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
// Sans ça : "Address already in use" si on relance le serveur trop vite.
// SO_REUSEADDR permet de réutiliser le port immédiatement.

struct sockaddr_in addr = {
    .sin_family      = AF_INET,
    .sin_addr.s_addr = INADDR_ANY,   // accepte les connexions sur toutes les interfaces
    .sin_port        = htons(PORT),  // htons = convertit en "network byte order" (big-endian)
};
```

#### La séquence socket serveur TCP

```
socket()   → crée le combiné
bind()     → attribue un numéro de téléphone (adresse + port)
listen()   → décroche et attend les appels (BACKLOG = file d'attente)
accept()   → répond à un appel entrant, retourne un NOUVEAU descripteur
             pour cette conversation spécifique
```

```c
while (1) {
    int client_fd = accept(server_fd, ...);
    // accept() BLOQUE ici jusqu'à ce qu'un client arrive.
    // server_fd = le "standard" qui écoute toujours
    // client_fd = le "téléphone" dédié à CE client

    handle_client(client_fd, &client_addr);
    close(client_fd);
    // Limitation : pendant handle_client (60 secondes), personne d'autre
    // ne peut se connecter. → Résolu dans server_concurrent.c
}
```

#### Envoi de l'heure

```c
for (int i = 0; i < 60; i++) {
    time_t t = time(NULL);      // secondes écoulées depuis le 1er janvier 1970
    char msg[64];
    strftime(msg, sizeof(msg), "Il est %H:%M:%S !\n", localtime(&t));
    // %H = heure, %M = minutes, %S = secondes
    send(client_fd, msg, strlen(msg), 0);
    sleep(1);
}
```

---

### 4.2. server_udp.c — Serveur UDP

#### Différences clés avec TCP

```c
// TCP                         UDP
socket(..., SOCK_STREAM, 0)    socket(..., SOCK_DGRAM, 0)
listen()                       // PAS de listen()
accept()                       // PAS d'accept()
recv()                         recvfrom()   // retourne aussi l'adresse expéditeur
send()                         sendto()     // il faut préciser la destination
```

#### Pourquoi recvfrom/sendto ?

En TCP, `accept()` crée un canal dédié avec le client — on sait déjà à qui on parle.
En UDP, pas de connexion : chaque datagramme peut venir d'un client différent.
`recvfrom()` retourne l'adresse de l'expéditeur pour qu'on sache où répondre avec `sendto()`.

```c
// Réception du "Bonjour" + récupération de l'adresse client
recvfrom(fd, buf, sizeof(buf)-1, 0, (struct sockaddr *)&client_addr, &client_len);

// Réponse vers cette même adresse
sendto(fd, msg, strlen(msg), 0, (struct sockaddr *)&client_addr, client_len);
```

---

### 4.3. server_concurrent.c — Serveur concurrent multi-services

#### Concept : fork()

```c
pid_t pid = fork();
// fork() duplique le processus en cours d'exécution.
// Les deux copies (père et fils) continuent à partir de la même ligne.
// La seule différence : la valeur retournée par fork().
//
//   Fils  → fork() retourne 0
//   Père  → fork() retourne le PID du fils (un entier > 0)
//   Erreur → fork() retourne -1

if (pid == 0) {
    // code exécuté UNIQUEMENT par le fils
} else {
    // code exécuté UNIQUEMENT par le père
}
```

#### Architecture à deux niveaux de fork

```
Niveau 1 — fork par service (au démarrage)
   main() fork → Service2 listener (processus dédié au port 8081)
   main() fork → Service3 listener (processus dédié au port 8082)
   main() devient → Service1 listener (port 8080)

Niveau 2 — fork par client (à chaque connexion)
   Service1 listener :
       accept() → fork() → fils gère le client (60 msgs)
                → père retourne à accept() immédiatement
```

#### `signal(SIGCHLD, SIG_IGN)`

```c
signal(SIGCHLD, SIG_IGN);
// Quand un fils se termine, il devient "zombie" si le père ne récupère pas
// son code de retour. SIG_IGN dit au noyau Linux de faire le ménage
// automatiquement. SIGCHLD = signal envoyé au père quand un fils meurt.
```

#### Pointeur de fonction

```c
static void run_listener(int server_fd, void (*handler)(int), const char *name)
//                                      ^^^^^^^^^^^^^^^^^^^^^
// "handler" est un pointeur vers une fonction qui prend un int et retourne void.
// On peut passer service1, service2 ou service3 comme argument.
// Équivalent d'un callback dans d'autres langages.
```

---

## 5. Côté Client

### 5.1. client_http.c — Client HTTP

Se connecte à un serveur HTTP (port 80 ou 8000 pour les tests locaux).
Lit une requête au clavier, gère la conversion `\n` → `\r\n` (obligatoire en HTTP),
envoie la requête et affiche la réponse.

**Point important** : HTTP exige `\r\n` à la fin de chaque ligne d'en-tête, pas juste `\n`.
Le code remplace automatiquement le `\n` du clavier par `\r\n`.

```
HEAD / HTTP/1.1\r\n
Host: localhost\r\n
\r\n               ← ligne vide obligatoire = fin des en-têtes HTTP
```

---

### 5.2. client_tcp.c — Client TCP

#### Séquence socket client TCP

```
socket()    → crée la socket
connect()   → initie le 3-way handshake TCP (SYN → SYN-ACK → ACK)
send()      → envoie "Bonjour"
recv()      → reçoit les messages du serveur en boucle
close()     → ferme la connexion
```

#### Le piège du compteur

```c
int count = 0;
while ((n = recv(fd, buf, sizeof(buf) - 1, 0)) > 0) {
    buf[n] = '\0';
    printf("%s", buf);
    count++;   // compte les appels recv(), pas les messages logiques !
}
printf("recv() appelé %d fois\n", count);
```

**Sans `sleep(1)` sur le serveur** : le serveur envoie tout très vite.
TCP peut regrouper plusieurs `send()` en un seul segment → `recv()` peut retourner
plusieurs messages en une fois → `count` sera petit (peut-être 1 ou 2).

**Avec `sleep(1)` sur le serveur** : chaque message arrive séparément,
espacé d'une seconde → `count` sera proche de 61 (60 heures + "Au revoir").

C'est exactement la question posée par le TP : TCP est un **flux**, pas des messages.

---

### 5.3. client_udp.c — Client UDP

```c
// Pas de connect() — UDP n'établit pas de connexion

sendto(fd, "Bonjour\n", 8, 0, (struct sockaddr *)&server_addr, server_len);
// Ce datagramme donne au serveur l'adresse (IP + port) de notre socket.

while (1) {
    int n = recvfrom(fd, buf, sizeof(buf)-1, 0, NULL, NULL);
    // En UDP : 1 recvfrom() = exactement 1 datagramme = 1 message.
    // count correspondra donc réellement au nombre de messages.
    if (strstr(buf, "Au revoir")) break;
    // En UDP il n'y a pas de fermeture de connexion → on détecte la fin
    // en cherchant "Au revoir" dans le contenu.
}
```

---

### 5.4. client_service1/2/3.c — Clients de test

Ces clients ajoutent un **timestamp** au début et à la fin pour prouver la concurrence :

```c
static void print_timestamp(const char *label)
{
    time_t t = time(NULL);
    char ts[32];
    strftime(ts, sizeof(ts), "%H:%M:%S", localtime(&t));
    printf("[%s] %s (PID=%d)\n", ts, label, getpid());
}
```

Si on lance `client_service1` et `client_service2` simultanément :
- `client_service1` commence à 14:00:00 et finit à 14:01:00 (60 secondes)
- `client_service2` doit commencer ET finir pendant ces 60 secondes

Si `client_service2` attendait la fin de `client_service1`, le serveur ne serait pas concurrent.

| Service | Port | Ce que fait le client |
|---------|------|----------------------|
| 1 | 8080 | envoie "Bonjour", reçoit heure x60 + "Au revoir" |
| 2 | 8081 | se connecte, reçoit le nombre de processus |
| 3 | 8082 | se connecte, reçoit `shared_file.txt`, sauvegarde en `received_file.txt` |

---

## 6. Flux complet des échanges

### TCP — Phases 3 & 5 (service 1)

```
CLIENT                          SERVEUR
  │                                │
  │──── SYN ──────────────────────►│  ┐
  │◄─── SYN-ACK ──────────────────│  │ 3-way handshake TCP
  │──── ACK ──────────────────────►│  ┘ (connect() déclenche tout ça)
  │                                │  accept() retourne
  │──── "Bonjour\n" ─────────────►│  recv() lit "Bonjour"
  │                                │
  │◄─── "Il est 14:00:01 !\n" ───│  send() + sleep(1)
  │◄─── "Il est 14:00:02 !\n" ───│  send() + sleep(1)
  │             ...  (x60)         │
  │◄─── "Il est 14:01:00 !\n" ───│
  │                                │
  │◄─── "Au revoir\n" ────────────│  send()
  │                                │  close(client_fd)
  │◄─── FIN ──────────────────────│  ┐ fermeture TCP
  │──── FIN-ACK ──────────────────►│  ┘ (déclenché par close())
```

### UDP — Phase 4

```
CLIENT                          SERVEUR
  │                                │
  │══ "Bonjour\n" ════════════════►│  recvfrom() → récupère addr client
  │                                │  (pas de connexion établie)
  │◄══ "Il est 14:00:01 !\n" ════│  sendto() + sleep(1)
  │◄══ "Il est 14:00:02 !\n" ════│
  │             ...  (x60)         │
  │◄══ "Au revoir\n" ═════════════│
  │   strstr() détecte "Au revoir" │
  │   → break, close()             │

  (══ = datagramme UDP, sans garantie d'ordre ni de livraison)
```

### Serveur concurrent — Phase 5

```
main()
  │ make_server(8080,8081,8082)
  │ fork() ──────────────────────────────────► Service2 listener
  │ fork() ─────────────────────────────────────────────────────► Service3 listener
  │ (devient Service1 listener)
  │
  │        Client A ──► accept() ──► fork() ──► fils A (60s, heure)
  │                                  │
  │        Client B ──► accept() ──► fork() ──► fils B (60s, heure)  ← simultané !
  │
  │                           Client C ──► accept() ──► fork() ──► fils C (ps aux)
  │                                                                 (répond en < 1s)
```

---

## 7. Commandes pour tester

### Compiler

```bash
cd server && make
cd Client && make
```

### Phase 2 — Client HTTP

```bash
# Tester sur le serveur du labo
telnet 10.250.101.1 80
# Taper : HEAD / HTTP/1.1
#         Host: localhost
#         [ligne vide]

# Tester en local
cd Client && ./client_http
```

### Phase 3 — TCP

```bash
# Terminal 1
cd server && ./server_tcp

# Terminal 2
cd Client && ./client_tcp
# Ou avec netcat :
nc localhost 8080    # taper "Bonjour" + Entrée
```

### Phase 4 — UDP

```bash
# Terminal 1
cd server && ./server_udp

# Terminal 2
cd Client && ./client_udp
# Ou avec netcat :
echo "Bonjour" | nc -u localhost 8080
```

### Phase 5 — Concurrent

```bash
# Terminal 1
cd server && ./server_concurrent

# Test d'un service seul
cd Client && ./client_service1   # reçoit l'heure 60 fois
cd Client && ./client_service2   # reçoit le nombre de processus
cd Client && ./client_service3   # reçoit shared_file.txt → received_file.txt

# Test de concurrence (lance les 3 en parallèle)
cd Client && ./test_parallel.sh
# Vérifier que les timestamps de service1 et service2/3 se chevauchent
```

### Capturer avec tcpdump / Wireshark

```bash
# Voir tous les segments TCP sur lo:8080
sudo tcpdump -i lo tcp port 8080

# Voir les données en clair (ASCII)
sudo tcpdump -i lo -A tcp port 8080

# Filtre Wireshark :
# Interface : lo
# Filtre    : tcp.port == 8080
```
