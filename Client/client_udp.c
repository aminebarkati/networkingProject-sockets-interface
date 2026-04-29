/*
 * =============================================================================
 * client_udp.c — Client UDP (mode non connecté)
 * =============================================================================
 *
 * Partie 4 du TP — Transfert de messages en mode non connecté
 *
 * Description :
 *   Envoie "Bonjour" au serveur UDP via sendto(), puis reçoit les messages
 *   d'heure et "Au revoir" via recvfrom().
 *
 *   DIFFÉRENCES CLEFS vs TCP :
 *   - Pas de connect() : UDP n'établit pas de connexion
 *   - sendto() / recvfrom() précisent l'adresse destination à chaque fois
 *   - 1 sendto() = 1 datagramme = 1 recvfrom() côté serveur (pas de fusion)
 *   - Les datagrammes peuvent arriver dans le désordre ou être perdus
 *   - Le count ici correspond réellement au nombre de messages reçus
 *
 * Usage :
 *   Compilation : gcc -o client_udp client_udp.c
 *   Exécution   : ./client_udp
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

/* ─── Main ─────────────────────────────────────────────────────────────────── */

int main(void)
{
    printf("=== Client UDP ===\n\n");

    /* Étape 1 : créer la socket UDP */
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("Erreur socket()"); exit(EXIT_FAILURE); }

    /* Préparer l'adresse du serveur (réutilisée dans chaque sendto/recvfrom) */
    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port   = htons(SERVER_PORT);
    if (inet_pton(AF_INET, SERVER_IP, &server_addr.sin_addr) <= 0) {
        perror("Erreur inet_pton()"); exit(EXIT_FAILURE);
    }
    socklen_t server_len = sizeof(server_addr);

    printf("[OK] Socket UDP créée\n\n");

    /* Étape 2 : envoyer "Bonjour" — ce datagramme donne au serveur notre adresse */
    const char *bonjour = "Bonjour\n";
    sendto(fd, bonjour, strlen(bonjour), 0,
           (struct sockaddr *)&server_addr, server_len);
    printf("[Envoyé] Bonjour (UDP)\n\n");

    /* Étape 3 : recevoir les réponses datagramme par datagramme */
    char buf[BUF_SIZE];
    int  count = 0;

    printf("--- Messages du serveur ---\n");

    while (1) {
        int n = recvfrom(fd, buf, sizeof(buf) - 1, 0, NULL, NULL);
        if (n <= 0) break;
        buf[n] = '\0';
        printf("%s", buf);
        count++;

        /* En UDP, on doit détecter la fin du protocole applicatif manuellement
           (pas de fermeture de connexion comme en TCP). */
        if (strstr(buf, "Au revoir") != NULL) break;
    }

    printf("\n--- Fin de l'échange ---\n");
    printf("Datagrammes reçus : %d (en UDP, 1 recvfrom = 1 message)\n", count);

    close(fd);
    return EXIT_SUCCESS;
}
