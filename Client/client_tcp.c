/*
 * =============================================================================
 * client_tcp.c — Client TCP (mode connecté)
 * =============================================================================
 *
 * Partie 3 du TP — Transfert de messages en mode connecté
 *
 * Description :
 *   Se connecte au serveur TCP sur localhost:8080, envoie "Bonjour",
 *   reçoit 60 messages d'heure + "Au revoir", et affiche combien
 *   de fois recv() a été appelé.
 *
 *   OBSERVATION CLEF : le nombre d'appels recv() ≠ nombre de messages.
 *   TCP est un flux continu — plusieurs send() côté serveur peuvent
 *   arriver en un seul recv() côté client (fusion), surtout sans sleep.
 *   Avec sleep(1) sur le serveur, chaque message arrive séparément.
 *
 * Usage :
 *   Compilation : gcc -o client_tcp client_tcp.c
 *   Exécution   : ./client_tcp
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

/* ─── Configuration ────────────────────────────────────────────────────────── */

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8080
#define BUF_SIZE    256

/* ─── Prototypes ───────────────────────────────────────────────────────────── */

int creer_socket_tcp(void);
void connecter_serveur(int fd, const char *ip, int port);

/* ─── Main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== Client TCP ===\n\n");

    /* Étape 1 : créer la socket et se connecter */
    int fd = creer_socket_tcp();
    connecter_serveur(fd, SERVER_IP, SERVER_PORT);
    printf("[OK] Connecté à %s:%d\n\n", SERVER_IP, SERVER_PORT);

    /* Étape 2 : envoyer "Bonjour" pour initier l'échange */
    const char *bonjour = "Bonjour\n";
    send(fd, bonjour, strlen(bonjour), 0);
    printf("[Envoyé] Bonjour\n\n");

    /* Étape 3 : recevoir les messages du serveur en boucle */
    char buf[BUF_SIZE];
    int  n, count = 0;

    printf("--- Messages du serveur ---\n");

    while ((n = recv(fd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
        count++;
        /* NOTE : count compte les appels recv(), pas les messages logiques.
           Chaque appel peut contenir plusieurs lignes si le réseau les a fusionnées. */
    }

    printf("\n--- Fin de l'échange ---\n");
    printf("recv() appelé %d fois (TCP peut fusionner plusieurs messages)\n", count);

    close(fd);
    return EXIT_SUCCESS;
}

/* ─── Implémentation ───────────────────────────────────────────────────────── */

int creer_socket_tcp(void)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("Erreur socket()"); exit(EXIT_FAILURE); }
    return fd;
}

void connecter_serveur(int fd, const char *ip, int port)
{
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("Erreur inet_pton()"); exit(EXIT_FAILURE);
    }
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("Erreur connect()"); exit(EXIT_FAILURE);
    }
}
