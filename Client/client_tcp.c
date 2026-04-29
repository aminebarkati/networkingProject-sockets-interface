/*
 * =============================================================================
 * client_tcp.c — Client TCP (mode connecté)
 * =============================================================================
 *
 * Partie 3 du TP — Transfert de messages en mode connecté
 *
 * Description :
 *   Ce programme se connecte au serveur TCP et reçoit 60 messages contenant
 *   l'heure courante. Il compte les messages reçus et les affiche.
 *
 *   Protocole d'échange attendu :
 *     1. Réception de "Bonjour\n"
 *     2. Réception de 60 fois "Il est HH:MM:SS !\n"
 *     3. Réception de "Au revoir\n"
 *
 * Usage :
 *   Compilation : gcc -o client_tcp client_tcp.c
 *   Exécution   : ./client_tcp
 *
 * Pour tester sans le serveur A, utiliser netcat en mode écoute :
 *   nc -l 8080
 *   (puis taper des messages manuellement)
 *
 * Protocole utilisé : TCP (SOCK_STREAM)
 * =============================================================================
 */

#define _POSIX_C_SOURCE 199309L
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>

/* ─── Configuration ───────────────────────────────────────────────────────── */

#define SERVER_IP   "127.0.0.1"   /* Localhost — changer par l'IP du serveur */
#define SERVER_PORT 8080          /* Port utilisé par server_tcp.c            */
#define BUF_SIZE    256           /* Taille du buffer de réception            */

/* ─── Prototypes ──────────────────────────────────────────────────────────── */

int  creer_socket_tcp(void);
void connecter_serveur(int fd, const char *ip, int port);
int  recevoir_message(int fd, char *buf, int taille);
void afficher_statistiques(int nb_messages, double duree_sec);

/* ─── Main ────────────────────────────────────────────────────────────────── */

int main(void) {
    int    socket_fd;
    char   buf[BUF_SIZE];
    int    n;
    int    nb_messages    = 0;     /* Compteur de messages reçus              */
    int    reception_finie = 0;    /* Flag de fin de session                  */

    /* Pour mesurer la durée totale de réception */
    struct timespec t_debut, t_fin;

    printf("=== Client TCP — Réception de l'heure ===\n\n");

    /* Étape 1 : Créer la socket */
    socket_fd = creer_socket_tcp();
    printf("[OK] Socket TCP créée (fd=%d)\n", socket_fd);

    /* Étape 2 : Se connecter au serveur */
    connecter_serveur(socket_fd, SERVER_IP, SERVER_PORT);
    printf("[OK] Connecté à %s:%d\n\n", SERVER_IP, SERVER_PORT);

    clock_gettime(CLOCK_MONOTONIC, &t_debut);

    /* Étape 3 : Boucle de réception des messages */
    printf("--- Messages reçus ---\n");

    while (!reception_finie) {
        n = recevoir_message(socket_fd, buf, BUF_SIZE);

        if (n <= 0) {
            /* n == 0 : connexion fermée proprement par le serveur   */
            /* n  < 0 : erreur réseau                                */
            if (n == 0) {
                printf("\n[INFO] Connexion fermée par le serveur.\n");
            } else {    
                perror("Erreur recv()");
            }
            break;
        }

        /*
         * ATTENTION — Comportement TCP important :
         * recv() peut retourner PLUSIEURS messages en un seul appel
         * (fusion TCP / algorithme de Nagle), ou UN message en plusieurs
         * appels (fragmentation). Ici on compte les appels à recv() qui
         * retournent des données, pas les messages logiques.
         *
         * Pour compter les messages logiques, il faudrait parser le buffer
         * sur les '\n'. On garde le comptage simple pour l'observation.
         */
        nb_messages++;
        printf("[msg %3d] %s", nb_messages, buf);
        fflush(stdout);  /* Forcer l'affichage immédiat */

        /* Détecter le message "Au revoir" = fin de session */
        if (strstr(buf, "Au revoir") != NULL) {
            reception_finie = 1;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t_fin);

    /* Étape 4 : Afficher les statistiques */
    double duree = (t_fin.tv_sec  - t_debut.tv_sec) +
                   (t_fin.tv_nsec - t_debut.tv_nsec) / 1e9;
    afficher_statistiques(nb_messages, duree);

    /* Étape 5 : Fermer la socket */
    close(socket_fd);
    printf("[OK] Socket fermée.\n");

    return EXIT_SUCCESS;
}

/* ─── Implémentation ──────────────────────────────────────────────────────── */

/*
 * creer_socket_tcp()
 * ------------------
 * Crée une socket TCP IPv4.
 */
int creer_socket_tcp(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) {
        perror("Erreur socket()");
        exit(EXIT_FAILURE);
    }
    return fd;
}

/*
 * connecter_serveur()
 * -------------------
 * Établit la connexion TCP avec le serveur (3-way handshake).
 * Si le serveur n'est pas démarré, connect() échouera immédiatement
 * avec "Connection refused".
 */
void connecter_serveur(int fd, const char *ip, int port) {
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));

    addr.sin_family      = AF_INET;
    addr.sin_port        = htons(port);

    if (inet_pton(AF_INET, ip, &addr.sin_addr) <= 0) {
        perror("Erreur inet_pton()");
        exit(EXIT_FAILURE);
    }

    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("Erreur connect() — Le serveur est-il démarré ?");
        exit(EXIT_FAILURE);
    }
}

/*
 * recevoir_message()
 * ------------------
 * Wrapper autour de recv() qui ajoute le '\0' terminal et retourne
 * le nombre d'octets reçus (0 = connexion fermée, -1 = erreur).
 */
int recevoir_message(int fd, char *buf, int taille) {
    int n = recv(fd, buf, taille - 1, 0);
    if (n > 0) {
        buf[n] = '\0';
    }
    return n;
}

/*
 * afficher_statistiques()
 * -----------------------
 * Affiche un résumé de la session : messages reçus, durée, débit.
 *
 * Observations attendues :
 * - SANS sleep côté serveur : TCP fusionne les messages (Nagle),
 *   on reçoit peu d'appels recv() avec beaucoup de données chacun.
 *   nb_messages (appels recv) << 60.
 *
 * - AVEC sleep(1) côté serveur : 1 message par seconde, les données
 *   arrivent espacées donc TCP ne peut pas fusionner.
 *   nb_messages ≈ 62 (bonjour + 60 heures + au revoir).
 */
void afficher_statistiques(int nb_messages, double duree_sec) {
    printf("\n========================================\n");
    printf("        STATISTIQUES DE RÉCEPTION\n");
    printf("========================================\n");
    printf("  Appels recv() ayant retourné des données : %d\n", nb_messages);
    printf("  Durée totale                              : %.2f secondes\n", duree_sec);
    printf("\n");
    printf("  INTERPRÉTATION :\n");
    printf("  Si nb_appels << 62 : TCP a fusionné des messages (Nagle)\n");
    printf("  Si nb_appels ≈ 62  : 1 message par appel (avec sleep)\n");
    printf("========================================\n");
}
