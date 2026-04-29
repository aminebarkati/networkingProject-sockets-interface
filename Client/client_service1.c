/*
 * =============================================================================
 * client_service1.c — Client du Service 1 (heure, port 8080)
 * =============================================================================
 *
 * Partie 5 du TP — Client de test pour le serveur concurrent
 *
 * Description :
 *   Se connecte au Service 1 du serveur concurrent (port 8080).
 *   Envoie "Bonjour", reçoit 60 messages d'heure puis "Au revoir".
 *   Affiche un timestamp au début et à la fin pour vérifier la concurrence :
 *   si deux instances tournent simultanément, leurs timestamps doivent
 *   se chevaucher (preuves qu'elles sont bien traitées en parallèle).
 *
 * Usage :
 *   Compilation : gcc -o client_service1 client_service1.c
 *   Exécution   : ./client_service1
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8080
#define BUF_SIZE    256

static void print_timestamp(const char *label)
{
    time_t t = time(NULL);
    char ts[32];
    strftime(ts, sizeof(ts), "%H:%M:%S", localtime(&t));
    printf("[%s] %s (PID=%d)\n", ts, label, getpid());
}

int main(void)
{
    printf("=== Client Service 1 — Heure (port %d) ===\n\n", SERVER_PORT);
    print_timestamp("Connexion");

    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(EXIT_FAILURE); }

    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SERVER_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("connect"); exit(EXIT_FAILURE);
    }

    send(fd, "Bonjour\n", 8, 0);

    char buf[BUF_SIZE];
    int  n, count = 0;

    while ((n = recv(fd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
        count++;
    }

    print_timestamp("Déconnexion");
    printf("recv() appelé %d fois\n", count);

    close(fd);
    return EXIT_SUCCESS;
}
