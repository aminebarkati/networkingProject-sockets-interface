# TP Réseaux — Architecture & Explications

## Table des matières
1. [Vue d'ensemble du projet](#1-vue-densemble-du-projet)
2. [C'est quoi une socket ?](#2-cest-quoi-une-socket-)
3. [Le Makefile expliqué](#3-le-makefile-expliqué)
4. [server_tcp.c — Serveur TCP séquentiel](#4-server_tcpc--serveur-tcp-séquentiel)
5. [server_udp.c — Serveur UDP](#5-server_udpc--serveur-udp)
6. [server_concurrent.c — Serveur concurrent multi-services](#6-server_concurrentc--serveur-concurrent-multi-services)
7. [Flux complet des échanges](#7-flux-complet-des-échanges)
8. [Commandes pour tester](#8-commandes-pour-tester)

---

## 1. Vue d'ensemble du projet

```
networkingProject-sockets-interface/
├── server_tcp.c          ← Phase 3 : serveur TCP, 1 client à la fois
├── server_udp.c          ← Phase 4 : serveur UDP, sans connexion
├── server_concurrent.c   ← Phase 5 : serveur concurrent, 3 services
├── shared_file.txt       ← fichier servi par le service 3 (port 8082)
└── Makefile              ← outil de compilation
```

Le binôme (côté client) fournira :
```
client_tcp.c
client_udp.c
client_service1.c / client_service2.c / client_service3.c
```

Les deux parties se connectent via le réseau local ou la boucle locale (`localhost`).

---

## 2. C'est quoi une socket ?

Une **socket** est un point de communication entre deux programmes sur un réseau.
Imaginez un téléphone : une socket, c'est le combiné. Vous composez un numéro (adresse IP + port),
la ligne s'établit, et vous pouvez envoyer/recevoir des données.

En C, une socket est simplement un **entier** (un `int`) que le système vous donne et qui représente
ce canal de communication. On appelle ça un **descripteur de fichier réseau** — comme un `fopen()`
mais pour le réseau.

### TCP vs UDP

| | TCP | UDP |
|---|---|---|
| Connexion | Oui (comme un appel téléphonique) | Non (comme un SMS) |
| Ordre garanti | Oui | Non |
| Pertes possibles | Non (retransmission auto) | Oui |
| Vitesse | Plus lent | Plus rapide |
| Usage typique | HTTP, SSH, FTP | DNS, streaming, jeux |

---

## 3. Le Makefile expliqué

```makefile
CC      = gcc          # compilateur à utiliser
CFLAGS  = -Wall -Wextra -g   # options de compilation

TARGETS = server_tcp server_udp server_concurrent  # noms des exécutables à produire

all: $(TARGETS)        # "make" ou "make all" compile les 3

server_tcp: server_tcp.c
	$(CC) $(CFLAGS) -o $@ $^
#   ^gcc  ^options  ^ ^  ^
#                   | |  └─ $^ = les fichiers source (server_tcp.c)
#                   | └──── $@ = le nom de la cible (server_tcp)
#                   └────── -o = "output" : nom de l'exécutable produit

server_udp: server_udp.c
	$(CC) $(CFLAGS) -o $@ $^

server_concurrent: server_concurrent.c
	$(CC) $(CFLAGS) -o $@ $^

clean:                 # "make clean" supprime les exécutables
	rm -f $(TARGETS)

.PHONY: all clean      # "all" et "clean" ne sont pas des fichiers, juste des commandes
```

### Options de compilation

| Option | Signification |
|--------|--------------|
| `-Wall` | Affiche tous les warnings (erreurs non bloquantes) |
| `-Wextra` | Affiche encore plus de warnings |
| `-g` | Ajoute les infos de débogage (utile avec `gdb`) |

### Utilisation

```bash
make                   # compile les 3 serveurs
make server_tcp        # compile uniquement server_tcp
make clean             # supprime les exécutables
```

---

## 4. server_tcp.c — Serveur TCP séquentiel

### Les includes (bibliothèques)

```c
#include <stdio.h>        // printf, perror
#include <stdlib.h>       // exit
#include <string.h>       // strlen
#include <unistd.h>       // close, sleep
#include <time.h>         // time, strftime, localtime
#include <sys/socket.h>   // socket, bind, listen, accept, send, recv
#include <netinet/in.h>   // struct sockaddr_in, htons
#include <arpa/inet.h>    // inet_ntoa (convertit IP binaire en texte)
```

### Les constantes

```c
#define PORT    8080   // numéro de port d'écoute
#define BACKLOG 10     // taille de la file d'attente des connexions entrantes
                       // (combien de clients peuvent "sonner" avant qu'on décroche)
```

### La fonction principale `main`

```c
int server_fd = socket(AF_INET, SOCK_STREAM, 0);
//              ^       ^        ^
//              |       |        └─ protocole (0 = automatique → TCP)
//              |       └────────── SOCK_STREAM = flux continu = TCP
//              └────────────────── AF_INET = réseau IPv4
// Retourne un entier (descripteur) ou -1 si erreur
```

```c
int opt = 1;
setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));
// Sans cette ligne, si vous relancez le serveur trop vite après l'avoir arrêté,
// vous obtenez "Address already in use". SO_REUSEADDR permet de réutiliser
// le port immédiatement.
```

```c
struct sockaddr_in addr = {
    .sin_family      = AF_INET,       // IPv4
    .sin_addr.s_addr = INADDR_ANY,    // accepte les connexions sur toutes les interfaces
                                      // (loopback + réseau local + etc.)
    .sin_port        = htons(PORT),   // htons = "host to network short"
                                      // convertit le numéro de port en ordre réseau
                                      // (les octets sont dans l'ordre inverse sur x86)
};
```

```c
bind(server_fd, (struct sockaddr *)&addr, sizeof(addr));
// "Attache" la socket à l'adresse/port définis ci-dessus.
// Le cast (struct sockaddr *) est nécessaire car bind() est générique
// et accepte plusieurs types d'adresses (IPv4, IPv6...).
```

```c
listen(server_fd, BACKLOG);
// Passe la socket en mode "écoute" : elle attend des connexions entrantes.
// BACKLOG = 10 : jusqu'à 10 clients peuvent attendre dans la file.
```

```c
while (1) {
    int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
    // accept() BLOQUE ici jusqu'à ce qu'un client se connecte.
    // Quand un client arrive, il retourne un NOUVEAU descripteur (client_fd)
    // spécifique à ce client. server_fd continue d'écouter d'autres connexions.
    
    handle_client(client_fd, &client_addr);
    close(client_fd);   // ferme la connexion avec ce client
}
```

### La fonction `handle_client`

```c
n = recv(client_fd, buf, sizeof(buf) - 1, 0);
// Reçoit des données du client dans buf (tableau de caractères).
// n = nombre d'octets reçus, 0 si le client a fermé, -1 si erreur.
// sizeof(buf) - 1 : on laisse une place pour le '\0' final (terminaison de chaîne C).
buf[n] = '\0';   // termine la chaîne pour pouvoir utiliser printf
```

```c
for (int i = 0; i < 60; i++) {
    time_t t = time(NULL);    // récupère l'heure actuelle (secondes depuis 1970)
    char msg[64];
    strftime(msg, sizeof(msg), "Il est %H:%M:%S !\n", localtime(&t));
    // strftime = "string format time" : formate l'heure en texte lisible
    // %H = heure, %M = minutes, %S = secondes
    
    if (send(client_fd, msg, strlen(msg), 0) < 0) break;
    // send() envoie msg au client. Si erreur (client déconnecté), on sort.
    
    sleep(1);    // attend 1 seconde
}

send(client_fd, "Au revoir\n", 10, 0);
// 10 = strlen("Au revoir\n"), passé en dur pour éviter un appel inutile
```

### Schéma de fonctionnement

```
server_tcp (séquentiel)
─────────────────────────────
socket()
bind()
listen()
      │
      ▼
   accept() ◄──────────────────────────────────┐
      │ client A se connecte                   │
      ▼                                        │
  recv("Bonjour")                              │
  send("Il est HH:MM:SS") x60                 │
  send("Au revoir")                            │
  close(client_fd)                             │
      │                                        │
      └────────────────────────────────────────┘
         client B attend pendant tout ce temps !
```

> **Limitation** : ce serveur ne peut servir qu'un client à la fois.
> Pendant que client A reçoit l'heure, client B est bloqué dans la file d'attente.

---

## 5. server_udp.c — Serveur UDP

### Différences clés avec TCP

UDP n'a pas de connexion. Il n'y a donc pas de `listen()` ni `accept()`.
On remplace `send`/`recv` par `sendto`/`recvfrom` qui incluent l'adresse.

```c
int fd = socket(AF_INET, SOCK_DGRAM, 0);
//                        ^
//                        SOCK_DGRAM = datagramme = UDP (pas de connexion)
```

```c
// Pas de listen() ni accept() ici !

int n = recvfrom(fd, buf, sizeof(buf) - 1, 0,
                 (struct sockaddr *)&client_addr, &client_len);
// recvfrom() BLOQUE jusqu'à recevoir un datagramme UDP.
// Il remplit client_addr avec l'adresse de l'expéditeur
// (on en a besoin pour répondre).
```

```c
sendto(fd, msg, strlen(msg), 0,
       (struct sockaddr *)&client_addr, client_len);
// sendto() envoie msg VERS client_addr.
// En TCP, send() sait déjà où envoyer (connexion établie).
// En UDP, il faut préciser la destination à chaque fois.
```

### Schéma

```
server_udp
──────────────────────────────
socket()
bind()
      │
      ▼
   recvfrom() ◄──────────────── client envoie "Bonjour" (UDP)
      │ récupère l'adresse client
      ▼
  sendto("Il est HH:MM:SS") x60
  sendto("Au revoir")
      │
      ▼
   recvfrom() ◄──────────────── attend un autre client
```

> **Attention** : En UDP, les paquets peuvent arriver dans le désordre ou être perdus.
> Le réseau local est fiable en pratique, mais ce n'est pas garanti par le protocole.

---

## 6. server_concurrent.c — Serveur concurrent multi-services

C'est le serveur le plus complexe. Il combine deux mécanismes :
1. **fork() par client** : chaque client est géré dans un processus fils
2. **fork() par service** : chaque service tourne dans son propre processus

### Concept : fork()

```c
pid_t pid = fork();
// fork() duplique le processus actuel.
// À partir de cet instant, DEUX processus exécutent le même code.
// La seule différence : la valeur de retour de fork().
//
//   Dans le fils  : fork() retourne 0
//   Dans le père  : fork() retourne le PID (numéro) du fils
//   Si erreur     : fork() retourne -1
```

Exemple simple pour comprendre :

```c
pid_t pid = fork();
if (pid == 0) {
    // CE CODE S'EXÉCUTE DANS LE FILS
    printf("Je suis le fils\n");
    exit(0);
}
// CE CODE S'EXÉCUTE DANS LE PÈRE
printf("Je suis le père, mon fils a le PID %d\n", pid);
```

### Structure du code

#### `make_server()` — fabrique une socket serveur

```c
static int make_server(int port)
{
    // socket() + setsockopt() + bind() + listen()
    // Même chose que server_tcp.c, mais dans une fonction réutilisable
    // car on doit créer 3 sockets (une par service).
    return fd;
}
```

#### Les 3 services

```c
// Service 1 — heure x60 (port 8080)
// Identique à server_tcp.c : recv "Bonjour", send heure x60, send "Au revoir"

// Service 2 — nombre de processus (port 8081)
static void service2(int client_fd)
{
    FILE *f = popen("ps aux | wc -l", "r");
    // popen() exécute une commande shell et retourne un FILE* pour lire sa sortie.
    // "ps aux" liste tous les processus, "wc -l" compte les lignes.
    fgets(result, sizeof(result), f);   // lit le résultat (un nombre)
    pclose(f);
    send(client_fd, msg, len, 0);       // envoie le résultat au client
}

// Service 3 — transfert de fichier (port 8082)
static void service3(int client_fd)
{
    FILE *f = fopen(FILE_PATH, "r");    // ouvre shared_file.txt
    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        send(client_fd, buf, n, 0);     // envoie le fichier par morceaux de 1024 octets
    fclose(f);
}
```

#### `run_listener()` — boucle d'écoute avec fork par client

```c
static void run_listener(int server_fd, void (*handler)(int), const char *name)
{
    signal(SIGCHLD, SIG_IGN);
    // Quand un fils se termine, il devient un "zombie" si le père ne l'attend pas.
    // SIG_IGN dit au système de nettoyer automatiquement les fils terminés.
    // SIGCHLD est le signal envoyé au père quand un fils se termine.

    while (1) {
        int client_fd = accept(server_fd, ...);   // attend un client

        pid_t pid = fork();
        if (pid == 0) {           // FILS : gère le client
            close(server_fd);     // le fils n'a pas besoin de la socket d'écoute
            handler(client_fd);   // appelle service1, service2, ou service3
            close(client_fd);
            exit(0);              // le fils se termine
        }
        close(client_fd);         // PÈRE : n'a plus besoin du client, retourne à accept()
    }
}
```

> `void (*handler)(int)` est un **pointeur de fonction** : on passe `service1`, `service2`,
> ou `service3` comme paramètre, et la fonction les appelle. C'est l'équivalent d'une
> fonction de callback.

#### `main()` — fork par service

```c
int main(void)
{
    int fd1 = make_server(8080);   // crée les 3 sockets serveur
    int fd2 = make_server(8081);
    int fd3 = make_server(8082);

    if (fork() == 0) {             // FILS 1 : gère le service 2
        close(fd1); close(fd3);    // ferme les sockets inutiles
        run_listener(fd2, service2, "Service2");
        exit(0);
    }

    if (fork() == 0) {             // FILS 2 : gère le service 3
        close(fd1); close(fd2);
        run_listener(fd3, service3, "Service3");
        exit(0);
    }

    close(fd2); close(fd3);        // PÈRE : gère le service 1
    run_listener(fd1, service1, "Service1");
}
```

### Arbre des processus

```
main (PID 100)
├── fork() → Service2 listener (PID 101)
│   ├── fork() → fils client A (PID 104)
│   └── fork() → fils client B (PID 107)  ← simultanés !
├── fork() → Service3 listener (PID 102)
│   └── fork() → fils client C (PID 105)
└── Service1 listener (PID 100 lui-même)
    ├── fork() → fils client D (PID 103)
    └── fork() → fils client E (PID 106)  ← simultanés !
```

---

## 7. Flux complet des échanges

### TCP (server_tcp + client_tcp)

```
CLIENT                          SERVEUR
  │                                │
  │──── SYN ──────────────────────►│  (TCP : établissement connexion)
  │◄─── SYN-ACK ──────────────────│
  │──── ACK ──────────────────────►│
  │                                │  accept() retourne
  │──── "Bonjour\n" ─────────────►│  recv() lit "Bonjour"
  │                                │
  │◄─── "Il est 14:00:01 !\n" ───│  send() + sleep(1)
  │◄─── "Il est 14:00:02 !\n" ───│  send() + sleep(1)
  │          ...  (x60)            │
  │◄─── "Il est 14:01:00 !\n" ───│
  │                                │
  │◄─── "Au revoir\n" ────────────│  send()
  │                                │  close(client_fd)
  │◄─── FIN ──────────────────────│  (TCP : fermeture)
  │──── FIN-ACK ──────────────────►│
  │                                │
```

### UDP (server_udp + client_udp)

```
CLIENT                          SERVEUR
  │                                │
  │══ "Bonjour\n" ════════════════►│  sendto() / recvfrom()
  │                                │  (pas de connexion établie !)
  │◄══ "Il est 14:00:01 !\n" ════│  sendto() + sleep(1)
  │◄══ "Il est 14:00:02 !\n" ════│
  │          ...  (x60)            │
  │◄══ "Au revoir\n" ═════════════│
  │                                │
  (══ = datagramme UDP, pas de garantie d'ordre ni de livraison)
```

### Concurrent (server_concurrent)

```
main                Service1          Service2          Service3
  │   fork()           │                  │                  │
  ├──────────────────►│  (PID 101)        │                  │
  │   fork()           │                  │                  │
  ├─────────────────────────────────────►│  (PID 102)        │
  │                    │                  │                  │
  │  (devient          │                  │                  │
  │   Service1)        │                  │                  │
  │                    │                  │                  │
  │             client A                  │                  │
  │             ──────►│fork()            │                  │
  │                    ├──► fils A        │                  │
  │                    │    (60 msgs)     │                  │
  │             client B                  │                  │
  │             ──────►│fork()            │                  │
  │                    ├──► fils B        │                  │  ← simultané !
  │                    │    (60 msgs)     │                  │
  │                              client C │                  │
  │                              ────────►│fork()            │
  │                                       ├──► fils C        │
  │                                       │    (ps aux)      │
```

---

## 8. Commandes pour tester

### Compiler

```bash
make              # compile les 3 serveurs
make clean        # supprime les exécutables
```

### Tester server_tcp (Phase 3)

```bash
# Terminal 1 : lancer le serveur
./server_tcp

# Terminal 2 : se connecter avec netcat
nc localhost 8080
# Taper "Bonjour" + Entrée → vous verrez l'heure s'afficher 60 fois
```

### Tester server_udp (Phase 4)

```bash
# Terminal 1
./server_udp

# Terminal 2
echo "Bonjour" | nc -u localhost 8080
# -u = mode UDP
```

### Tester server_concurrent (Phase 5)

```bash
# Terminal 1 : lancer le serveur concurrent
./server_concurrent

# Terminal 2 : service 1 (heure)
nc localhost 8080   # taper "Bonjour"

# Terminal 3 : service 1 en même temps → les deux reçoivent l'heure simultanément
nc localhost 8080   # taper "Bonjour"

# Terminal 4 : service 2 (processus)
nc localhost 8081

# Terminal 5 : service 3 (fichier)
nc localhost 8082
```

### Capturer avec tcpdump

```bash
# Capturer tout le trafic TCP sur le port 8080 (loopback)
sudo tcpdump -i lo tcp port 8080

# Voir les données en clair (ASCII)
sudo tcpdump -i lo -A tcp port 8080
```

### Capturer avec Wireshark

```
Interface : lo (loopback)
Filtre    : tcp.port == 8080
```
