/*
 * =============================================================================
 * client_http.c — Client HTTP en mode connecté (TCP)
 * =============================================================================
 *
 * Partie 2 du TP — Client/Serveur Sockets
 *
 * Description :
 *   Ce programme implémente un client HTTP simple utilisant l'interface
 *   sockets BSD. Il lit une requête HTTP au clavier, l'envoie au serveur
 *   HTTP, puis affiche la réponse complète.
 *
 *   C'est l'équivalent de faire : telnet <serveur> 80
 *   mais implémenté manuellement en C.
 *
 * Usage :
 *   Compilation : gcc -o client_http client_http.c
 *   Exécution   : ./client_http
 *
 * Exemple de requête à taper :
 *   HEAD / HTTP/1.1
 *   Host: localhost
 *   (ligne vide pour terminer)
 *
 * Protocole utilisé : TCP (mode connecté, SOCK_STREAM)
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>   /* socket(), connect(), send(), recv() */
#include <netinet/in.h>   /* sockaddr_in, htons(), htonl()       */
#include <arpa/inet.h>    /* inet_pton() pour convertir l'IP     */

/* ─── Configuration ───────────────────────────────────────────────────────── */

#define SERVER_IP   "127.0.0.1"  /* IP du serveur HTTP                    */
#define SERVER_PORT 8000              /* Port HTTP standard                     */
#define BUF_SIZE    4096            /* Taille du buffer de réception          */
#define REQ_SIZE    1024            /* Taille max d'une requête clavier       */

/* ─── Prototypes ──────────────────────────────────────────────────────────── */

int  creer_socket_tcp(void);
void connecter_serveur(int fd, const char *ip, int port);
void lire_requete_clavier(char *requete, int taille_max);
void envoyer_requete(int fd, const char *requete);
void recevoir_et_afficher_reponse(int fd);

/* ─── Main ────────────────────────────────────────────────────────────────── */

int main(void) {
    int  socket_fd;                 /* Descripteur de la socket              */
    char requete[REQ_SIZE];         /* Buffer pour la requête HTTP           */

    printf("=== Client HTTP (TCP) ===\n\n");

    /* Étape 1 : Créer la socket TCP */
    socket_fd = creer_socket_tcp();
    printf("[OK] Socket TCP créée (fd=%d)\n", socket_fd);

    /* Étape 2 : Se connecter au serveur HTTP */
    connecter_serveur(socket_fd, SERVER_IP, SERVER_PORT);
    printf("[OK] Connecté à %s:%d\n\n", SERVER_IP, SERVER_PORT);

    /* Étape 3 : Lire la requête HTTP au clavier */
    lire_requete_clavier(requete, REQ_SIZE);

    /* Étape 4 : Envoyer la requête au serveur */
    envoyer_requete(socket_fd, requete);
    printf("\n[...] Requête envoyée, attente de la réponse...\n\n");

    /* Étape 5 : Recevoir et afficher la réponse */
    recevoir_et_afficher_reponse(socket_fd);

    /* Étape 6 : Fermer la socket proprement */
    close(socket_fd);
    printf("\n[OK] Connexion fermée.\n");

    return EXIT_SUCCESS;
}

/* ─── Implémentation des fonctions ───────────────────────────────────────── */

/*
 * creer_socket_tcp()
 * ------------------
 * Crée une socket TCP (AF_INET = IPv4, SOCK_STREAM = TCP).
 * Retourne le file descriptor ou quitte en cas d'erreur.
 *
 * AF_INET     : famille d'adresses IPv4
 * SOCK_STREAM : type de socket TCP (orienté connexion, fiable, ordonné)
 * 0           : protocole automatique (TCP pour SOCK_STREAM)
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
 * Remplit la structure sockaddr_in et appelle connect() pour établir
 * la connexion TCP avec le serveur (3-way handshake : SYN/SYN-ACK/ACK).
 *
 * htons()   : convertit le port en "network byte order" (big-endian)
 * inet_pton : convertit l'IP "10.250.101.1" en binaire réseau
 */
void connecter_serveur(int fd, const char *ip, int port) {
    struct sockaddr_in adresse_serveur;

    /* Initialiser la structure à zéro pour éviter les valeurs parasites */
    memset(&adresse_serveur, 0, sizeof(adresse_serveur));

    adresse_serveur.sin_family = AF_INET;           /* IPv4                */
    adresse_serveur.sin_port   = htons(port);       /* Port en big-endian  */

    /* Convertir l'IP texte → binaire réseau */
    if (inet_pton(AF_INET, ip, &adresse_serveur.sin_addr) <= 0) {
        perror("Erreur inet_pton() — IP invalide");
        exit(EXIT_FAILURE);
    }

    if (connect(fd, (struct sockaddr *)&adresse_serveur,
                sizeof(adresse_serveur)) == -1) {
        perror("Erreur connect()");
        exit(EXIT_FAILURE);
    }
}

/*
 * lire_requete_clavier()
 * ----------------------
 * Lit les lignes tapées au clavier jusqu'à une ligne vide (Entrée seul),
 * qui signale la fin de l'en-tête HTTP. Ajoute \r\n comme le standard HTTP
 * l'exige (CRLF).
 *
 * Note HTTP : chaque ligne de l'en-tête se termine par \r\n
 *             et une ligne vide \r\n signale la fin de l'en-tête.
 */
void lire_requete_clavier(char *requete, int taille_max) {
    char ligne[256];

    printf("Entrez votre requête HTTP (ligne vide pour terminer) :\n");
    printf("Exemple : HEAD / HTTP/1.1\n");
    printf("          Host: localhost\n");
    printf("          [ligne vide]\n\n");

    requete[0] = '\0';  /* Initialiser le buffer */

    while (1) {
        if (fgets(ligne, sizeof(ligne), stdin) == NULL) break;

        /* Remplacer le \n final par \r\n (format HTTP) */
        int len = strlen(ligne);
        if (len > 0 && ligne[len-1] == '\n') {
            ligne[len-1] = '\0';
            strncat(requete, ligne, taille_max - strlen(requete) - 3);
            strncat(requete, "\r\n", taille_max - strlen(requete) - 1);
        }
        /* Une ligne vide = fin de l'en-tête HTTP */
        if (strcmp(ligne, "") == 0) {
            break;
        }
    }
}

/*
 * envoyer_requete()
 * -----------------
 * Envoie la requête complète via send().
 * send() peut ne pas tout envoyer en un seul appel (rare mais possible),
 * d'où la vérification du nombre d'octets envoyés.
 */
void envoyer_requete(int fd, const char *requete) {
    int len     = strlen(requete);
    int envoyes = send(fd, requete, len, 0);

    if (envoyes == -1) {
        perror("Erreur send()");
        exit(EXIT_FAILURE);
    }

    printf("[INFO] %d/%d octets envoyés\n", envoyes, len);
}

/*
 * recevoir_et_afficher_reponse()
 * ------------------------------
 * Reçoit la réponse du serveur en boucle jusqu'à ce que recv() retourne 0
 * (connexion fermée par le serveur) ou -1 (erreur).
 *
 * IMPORTANT : recv() ne garantit pas de recevoir tout d'un coup.
 * TCP peut fragmenter ou regrouper les données → d'où la boucle.
 *
 * Observation Wireshark : 1 send() côté serveur ≠ forcément 1 segment TCP.
 * TCP peut fusionner plusieurs petits envois (algorithme de Nagle).
 */
void recevoir_et_afficher_reponse(int fd) {
    char buf[BUF_SIZE];
    int  n;
    int  total = 0;

    printf("--- Réponse du serveur ---\n");

    while ((n = recv(fd, buf, BUF_SIZE - 1, 0)) > 0) {
        buf[n] = '\0';       /* Terminer la chaîne pour printf */
        printf("%s", buf);
        total += n;
    }

    if (n == -1) {
        perror("Erreur recv()");
    }

    printf("\n--- Fin de la réponse (%d octets reçus) ---\n", total);
}
