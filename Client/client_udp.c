/*
 * =============================================================================
 * client_udp.c — Client UDP (mode non connecté)
 * =============================================================================
 *
 * Partie 4 du TP — Transfert de messages en mode non connecté
 *
 * Description :
 *   Version UDP du client TCP. Contrairement à TCP, UDP est :
 *     - Sans connexion (pas de handshake)
 *     - Non fiable (pertes possibles, pas de retransmission)
 *     - Non ordonné (les paquets peuvent arriver dans le désordre)
 *     - Plus rapide (pas d'overhead de connexion)
 *
 *   Différences clés dans le code vs client_tcp.c :
 *     - SOCK_DGRAM au lieu de SOCK_STREAM
 *     - Pas de connect()
 *     - sendto() au lieu de send()
 *     - recvfrom() au lieu de recv()
 *
 * Usage :
 *   Compilation : gcc -o client_udp client_udp.c
 *   Exécution   : ./client_udp
 *
 * Pour tester sans le serveur A :
 *   nc -u -l 8081    (netcat en mode écoute UDP)
 *
 * Protocole utilisé : UDP (SOCK_DGRAM)
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

/* ─── Configuration ───────────────────────────────────────────────────────── */

#define SERVER_IP   "127.0.0.1"
#define SERVER_PORT 8081          /* Port différent du TCP pour éviter conflit */
#define BUF_SIZE    256
#define TIMEOUT_SEC 5             /* Timeout si un datagramme est perdu        */

/* ─── Prototypes ──────────────────────────────────────────────────────────── */

int  creer_socket_udp(void);
void configurer_timeout(int fd, int secondes);
void remplir_adresse(struct sockaddr_in *addr, const char *ip, int port);
int  recevoir_datagramme(int fd, char *buf, int taille,
                         struct sockaddr_in *addr_serveur);
void afficher_comparaison_tcp_udp(int nb_recus, int nb_attendus,
                                  double duree_sec);

/* ─── Main ────────────────────────────────────────────────────────────────── */

int main(void) {
    int               socket_fd;
    char              buf[BUF_SIZE];
    struct sockaddr_in addr_serveur;
    int               n;
    int               nb_messages_recus   = 0;
    int               nb_messages_attendus = 62; /* Bonjour + 60 heures + Au revoir */
    int               reception_finie     = 0;

    struct timespec t_debut, t_fin;

    printf("=== Client UDP — Mode non connecté ===\n\n");

    /* Étape 1 : Créer la socket UDP */
    socket_fd = creer_socket_udp();
    printf("[OK] Socket UDP créée (fd=%d)\n", socket_fd);

    /*
     * Pas de connect() en UDP pur !
     * On va utiliser sendto/recvfrom avec l'adresse du serveur à chaque fois.
     * Différence fondamentale avec TCP :
     *   TCP → connexion établie AVANT l'échange de données
     *   UDP → l'adresse de destination est fournie à CHAQUE envoi
     */

    /* Configurer un timeout sur recv pour détecter les paquets perdus */
    configurer_timeout(socket_fd, TIMEOUT_SEC);

    /* Préparer l'adresse du serveur (utilisée dans sendto/recvfrom) */
    remplir_adresse(&addr_serveur, SERVER_IP, SERVER_PORT);

    printf("[INFO] Serveur cible : %s:%d\n", SERVER_IP, SERVER_PORT);
    printf("[INFO] Timeout par datagramme : %d secondes\n\n", TIMEOUT_SEC);

    /*
     * En UDP, le client doit d'abord envoyer quelque chose pour que le
     * serveur connaisse son adresse (IP + port source).
     * On envoie un message "HELLO" pour initier la communication.
     */
    const char *init_msg = "HELLO\n";
    socklen_t addr_len = sizeof(addr_serveur);

    printf("[...] Envoi du message d'initialisation...\n");
    if (sendto(socket_fd, init_msg, strlen(init_msg), 0,
               (struct sockaddr *)&addr_serveur, addr_len) == -1) {
        perror("Erreur sendto()");
        exit(EXIT_FAILURE);
    }

    clock_gettime(CLOCK_MONOTONIC, &t_debut);

    /* Étape 2 : Boucle de réception des datagrammes */
    printf("--- Datagrammes reçus ---\n");

    while (!reception_finie) {
        n = recevoir_datagramme(socket_fd, buf, BUF_SIZE, &addr_serveur);

        if (n < 0) {
            /*
             * En UDP, une erreur recv peut signifier :
             *   - Timeout (EAGAIN/EWOULDBLOCK) → paquet perdu !
             *   - Erreur réseau réelle
             * Contrairement à TCP, on ne peut PAS détecter si le serveur
             * a planté — il n'y a pas de FIN/RST en UDP.
             */
            perror("[ATTENTION] recvfrom() — Timeout ou perte de paquet");
            printf("[INFO] Paquet perdu ou serveur arrêté. Fin de réception.\n");
            break;
        }

        if (n == 0) {
            printf("[INFO] Datagramme vide reçu.\n");
            continue;
        }

        nb_messages_recus++;
        printf("[dgram %3d] %s", nb_messages_recus, buf);
        fflush(stdout);

        /* Détecter "Au revoir" */
        if (strstr(buf, "Au revoir") != NULL) {
            reception_finie = 1;
        }
    }

    clock_gettime(CLOCK_MONOTONIC, &t_fin);

    /* Étape 3 : Afficher la comparaison TCP vs UDP */
    double duree = (t_fin.tv_sec  - t_debut.tv_sec) +
                   (t_fin.tv_nsec - t_debut.tv_nsec) / 1e9;

    afficher_comparaison_tcp_udp(nb_messages_recus, nb_messages_attendus, duree);

    close(socket_fd);
    printf("[OK] Socket fermée.\n");

    return EXIT_SUCCESS;
}

/* ─── Implémentation ──────────────────────────────────────────────────────── */

/*
 * creer_socket_udp()
 * ------------------
 * SOCK_DGRAM : socket UDP, sans connexion, basée sur des datagrammes.
 * Chaque appel sendto/recvfrom est indépendant.
 */
int creer_socket_udp(void) {
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd == -1) {
        perror("Erreur socket() UDP");
        exit(EXIT_FAILURE);
    }
    return fd;
}

/*
 * configurer_timeout()
 * --------------------
 * Configure un timeout sur les opérations de réception.
 * En UDP, si un datagramme est perdu, recvfrom() bloquerait indéfiniment
 * sans timeout. Avec SO_RCVTIMEO, il échoue après `secondes` secondes.
 *
 * En TCP ce n'est pas nécessaire : si la connexion est coupée, le kernel
 * le détecte via les keepalives ou le FIN/RST.
 */
void configurer_timeout(int fd, int secondes) {
    struct timeval timeout;
    timeout.tv_sec  = secondes;
    timeout.tv_usec = 0;

    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,
                   &timeout, sizeof(timeout)) == -1) {
        perror("Erreur setsockopt() SO_RCVTIMEO");
        exit(EXIT_FAILURE);
    }
}

/*
 * remplir_adresse()
 * -----------------
 * Remplit une structure sockaddr_in avec l'IP et le port.
 */
void remplir_adresse(struct sockaddr_in *addr, const char *ip, int port) {
    memset(addr, 0, sizeof(*addr));
    addr->sin_family = AF_INET;
    addr->sin_port   = htons(port);

    if (inet_pton(AF_INET, ip, &addr->sin_addr) <= 0) {
        perror("Erreur inet_pton()");
        exit(EXIT_FAILURE);
    }
}

/*
 * recevoir_datagramme()
 * ---------------------
 * Reçoit un datagramme UDP. Contrairement à TCP où recv() peut recevoir
 * des fragments, chaque recvfrom() reçoit exactement un datagramme complet.
 *
 * Propriété clé UDP : 1 sendto() côté serveur = 1 recvfrom() côté client
 * (pas de fusion possible contrairement à TCP et Nagle).
 */
int recevoir_datagramme(int fd, char *buf, int taille,
                        struct sockaddr_in *addr_serveur) {
    socklen_t addr_len = sizeof(*addr_serveur);
    int n = recvfrom(fd, buf, taille - 1, 0,
                     (struct sockaddr *)addr_serveur, &addr_len);
    if (n > 0) {
        buf[n] = '\0';
    }
    return n;
}

/*
 * afficher_comparaison_tcp_udp()
 * ------------------------------
 * Affiche une comparaison des comportements TCP vs UDP observés.
 */
void afficher_comparaison_tcp_udp(int nb_recus, int nb_attendus,
                                   double duree_sec) {
    int perdus = nb_attendus - nb_recus;
    if (perdus < 0) perdus = 0;

    printf("\n╔══════════════════════════════════════════════╗\n");
    printf("║        BILAN UDP vs TCP                      ║\n");
    printf("╠══════════════════════════════════════════════╣\n");
    printf("║  Messages attendus   : %-4d                  ║\n", nb_attendus);
    printf("║  Messages reçus      : %-4d                  ║\n", nb_recus);
    printf("║  Messages perdus     : %-4d                  ║\n", perdus);
    printf("║  Durée totale        : %.2f sec              ║\n", duree_sec);
    printf("╠══════════════════════════════════════════════╣\n");
    printf("║  Critère          TCP          UDP           ║\n");
    printf("║  ─────────────────────────────────────────  ║\n");
    printf("║  Connexion        Oui          Non           ║\n");
    printf("║  Pertes           Non          Oui possible  ║\n");
    printf("║  Ordre garanti    Oui          Non           ║\n");
    printf("║  Fusion messages  Possible     Jamais        ║\n");
    printf("║  Détection panne  Oui (FIN)    Non           ║\n");
    printf("║  Multi-clients    Difficile    Natif         ║\n");
    printf("╚══════════════════════════════════════════════╝\n");
}
