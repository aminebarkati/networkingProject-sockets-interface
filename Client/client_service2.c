/*
 * =============================================================================
 * client_service2.c — Client pour le Service 2 : nombre de processus
 * =============================================================================
 *
 * Partie 5 du TP — Serveur concurrent, multi-services
 *
 * Description :
 *   Client de test pour le Service 2 du serveur concurrent.
 *   Service 2 (port 8081) : interroge le serveur pour connaître le nombre
 *   de processus en cours d'exécution sur la machine serveur.
 *
 *   Le serveur exécute : popen("ps aux | wc -l", "r")
 *   et renvoie le résultat au client.
 *
 * Usage :
 *   Compilation : gcc -o client_service2 client_service2.c
 *   Exécution   : ./client_service2
 *
 * Port : 8081
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

#define SERVER_IP     "127.0.0.1"
#define SERVICE2_PORT 8081
#define BUF_SIZE      1024

int main(void) {
    int  fd;
    char buf[BUF_SIZE];
    int  n;
    struct sockaddr_in addr;

    printf("[PID %d] Service 2 — Interrogation du nombre de processus\n",
           getpid());
    printf("[PID %d] Connexion à %s:%d\n", getpid(), SERVER_IP, SERVICE2_PORT);

    /* Créer la socket TCP */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) { perror("socket"); exit(EXIT_FAILURE); }

    /* Configurer l'adresse du serveur */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SERVICE2_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    /* Se connecter */
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("[PID] connect() — Service 2 disponible ?");
        exit(EXIT_FAILURE);
    }

    printf("[PID %d] Connecté. Attente de la réponse du serveur...\n",
           getpid());

    /*
     * Ce service est simple : le serveur envoie le résultat dès la connexion,
     * pas besoin d'envoyer une requête. On reçoit et on affiche.
     *
     * Scénario côté serveur (server_concurrent.c) :
     *   FILE *f = popen("ps aux | wc -l", "r");
     *   fgets(result, sizeof(result), f);
     *   send(client_fd, result, strlen(result), 0);
     *   pclose(f);
     *   close(client_fd);
     */
    printf("\n--- Réponse du Serveur (Service 2 : nb processus) ---\n");

    while ((n = recv(fd, buf, BUF_SIZE - 1, 0)) > 0) {
        buf[n] = '\0';
        printf("[PID %d] Nombre de processus sur le serveur : %s",
               getpid(), buf);
        fflush(stdout);
    }

    printf("[PID %d] Connexion fermée par le serveur.\n", getpid());
    printf("--- Fin Service 2 ---\n\n");

    close(fd);
    return EXIT_SUCCESS;
}
