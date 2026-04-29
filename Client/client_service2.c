/*
 * =============================================================================
 * client_service2.c — Client du Service 2 (nombre de processus, port 8081)
 * =============================================================================
 *
 * Partie 5 du TP — Client de test pour le serveur concurrent
 *
 * Description :
 *   Se connecte au Service 2 du serveur concurrent (port 8081).
 *   Le serveur répond immédiatement avec le nombre de processus en cours
 *   sur sa machine (résultat de "ps aux | wc -l"), puis ferme la connexion.
 *
 * Usage :
 *   Compilation : gcc -o client_service2 client_service2.c
 *   Exécution   : ./client_service2
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
#define SERVER_PORT 8081
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
    printf("=== Client Service 2 — Processus (port %d) ===\n\n", SERVER_PORT);
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

    /* Le service 2 ne nécessite pas d'envoi initial : le serveur répond
       dès que la connexion est établie. */
    char buf[BUF_SIZE];
    int  n;

    printf("--- Réponse du serveur ---\n");
    while ((n = recv(fd, buf, sizeof(buf) - 1, 0)) > 0) {
        buf[n] = '\0';
        printf("%s", buf);
    }
    printf("--- Fin ---\n");

    print_timestamp("Déconnexion");

    close(fd);
    return EXIT_SUCCESS;
}
