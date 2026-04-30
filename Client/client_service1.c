/* Nécessaire pour CLOCK_MONOTONIC et clock_gettime() (POSIX.1b) */
#define _POSIX_C_SOURCE 199309L

/*
 * =============================================================================
 * client_service1.c — Client pour le Service 1 : réception de l'heure
 * =============================================================================
 *
 * Partie 5 du TP — Serveur concurrent, multi-services
 *
 * Description :
 *   Client de test pour le Service 1 du serveur concurrent.
 *   Service 1 (port 8080) : reçoit l'heure du serveur 60 fois.
 *
 *   Utilisé pour vérifier que le serveur concurrent traite bien plusieurs
 *   clients du MÊME service en parallèle (chacun fork() un processus fils).
 *
 * Usage :
 *   Compilation : gcc -o client_service1 client_service1.c
 *   Exécution   : ./client_service1
 *   En parallèle: ./client_service1 & ./client_service1 & ./client_service1 &
 *
 * Port : 8080
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

#define SERVER_IP   "127.0.0.1"
#define SERVICE1_PORT 8080
#define BUF_SIZE    256

/* ─── Prototype ───────────────────────────────────────────────────────────── */

void envoyer_message(int fd, const char *message);

/* ─── Main ────────────────────────────────────────────────────────────────── */

int main(void) {
    int  fd;
    char buf[BUF_SIZE];
    int  n, compteur = 0;
    struct sockaddr_in addr;
    struct timespec t0, t1;

    /* Afficher le PID pour distinguer les instances parallèles */
    printf("[PID %d] Service 1 — Connexion à %s:%d\n",
           getpid(), SERVER_IP, SERVICE1_PORT);

    /* Créer et connecter la socket TCP */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) { perror("socket"); exit(EXIT_FAILURE); }

    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SERVICE1_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("[PID] connect() — Service 1 disponible ?");
        exit(EXIT_FAILURE);
    }

    printf("[PID %d] Connecté. Réception en cours...\n", getpid());

    /* Envoyer un message d'initialisation au serveur */
    envoyer_message(fd, "BONJOUR\n");

    clock_gettime(CLOCK_MONOTONIC, &t0);

    /* Recevoir les messages */
    while ((n = recv(fd, buf, BUF_SIZE - 1, 0)) > 0) {
        buf[n] = '\0';
        compteur++;
        printf("[PID %d][msg %2d] %s", getpid(), compteur, buf);
        fflush(stdout);
        if (strstr(buf, "Au revoir")) break;
    }

    clock_gettime(CLOCK_MONOTONIC, &t1);
    double duree = (t1.tv_sec - t0.tv_sec) + (t1.tv_nsec - t0.tv_nsec) / 1e9;

    printf("[PID %d] Terminé : %d messages en %.1f secondes.\n",
           getpid(), compteur, duree);

    close(fd);
    return EXIT_SUCCESS;
}

/* ─── Implémentation ──────────────────────────────────────────────────────── */

/*
 * envoyer_message()
 * -----------------
 * Envoie un message texte au serveur via send().
 *
 * Boucle robuste : send() peut n'envoyer qu'une partie des données
 * si le buffer noyau est plein. La boucle garantit que tous les octets
 * sont transmis avant de retourner.
 *
 * Paramètres :
 *   fd      : file descriptor de la socket connectée
 *   message : chaîne à envoyer (terminée par '\0')
 */
void envoyer_message(int fd, const char *message) {
    int total   = strlen(message);
    int envoyes = 0;
    int n;

    while (envoyes < total) {
        n = send(fd, message + envoyes, total - envoyes, 0);

        if (n == -1) {
            perror("Erreur send() dans envoyer_message()");
            exit(EXIT_FAILURE);
        }

        envoyes += n;
    }

    printf("[PID %d][ENVOI] \"%.*s\" (%d octet%s)\n",
           getpid(),
           total - 1,   /* -1 pour ne pas afficher le \n final */
           message,
           total,
           total > 1 ? "s" : "");
}