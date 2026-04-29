/*
 * =============================================================================
 * client_service3.c — Client pour le Service 3 : transfert de fichier
 * =============================================================================
 *
 * Partie 5 du TP — Serveur concurrent, multi-services
 *
 * Description :
 *   Client de test pour le Service 3 du serveur concurrent.
 *   Service 3 (port 8082) : reçoit un fichier texte envoyé par le serveur
 *   et le sauvegarde localement sous "fichier_recu.txt".
 *
 *   Le serveur lit un fichier texte et envoie son contenu octet par octet
 *   ou par blocs. Le client reçoit tout et reconstruit le fichier.
 *
 * Usage :
 *   Compilation : gcc -o client_service3 client_service3.c
 *   Exécution   : ./client_service3
 *   Résultat    : fichier_recu_<PID>.txt
 *
 * Port : 8082
 * =============================================================================
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define SERVER_IP     "127.0.0.1"
#define SERVICE3_PORT 8082
#define BUF_SIZE      4096        /* Buffer plus grand pour le transfert fichier */

int main(void) {
    int   fd;
    char  buf[BUF_SIZE];
    int   n;
    long  total_octets = 0;
    struct sockaddr_in addr;

    /* Nom de fichier unique par PID pour éviter les conflits si lancé en parallèle */
    char nom_fichier[64];
    snprintf(nom_fichier, sizeof(nom_fichier), "fichier_recu_%d.txt", getpid());

    printf("[PID %d] Service 3 — Transfert de fichier\n", getpid());
    printf("[PID %d] Connexion à %s:%d\n", getpid(), SERVER_IP, SERVICE3_PORT);
    printf("[PID %d] Fichier de sortie : %s\n\n", getpid(), nom_fichier);

    /* Créer la socket TCP */
    fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd == -1) { perror("socket"); exit(EXIT_FAILURE); }

    /* Configurer l'adresse serveur */
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port   = htons(SERVICE3_PORT);
    inet_pton(AF_INET, SERVER_IP, &addr.sin_addr);

    /* Se connecter */
    if (connect(fd, (struct sockaddr *)&addr, sizeof(addr)) == -1) {
        perror("[PID] connect() — Service 3 disponible ?");
        exit(EXIT_FAILURE);
    }

    printf("[PID %d] Connecté. Réception du fichier en cours...\n", getpid());

    /* Ouvrir le fichier de sortie en écriture */
    FILE *fichier_sortie = fopen(nom_fichier, "w");
    if (fichier_sortie == NULL) {
        perror("fopen() — Impossible de créer le fichier de sortie");
        close(fd);
        exit(EXIT_FAILURE);
    }

    /*
     * Boucle de réception et écriture dans le fichier.
     *
     * Important TCP : recv() peut retourner des blocs de taille variable.
     * On ne peut PAS supposer qu'un recv() correspond à un seul segment.
     * On écrit chaque bloc reçu dans le fichier jusqu'à ce que le serveur
     * ferme la connexion (recv retourne 0).
     *
     * C'est justement l'avantage de TCP pour le transfert de fichier :
     * on est sûr que tous les octets arrivent, dans l'ordre, sans perte.
     * En UDP il faudrait un protocole applicatif de plus (numéros de séquence,
     * accusés de réception, etc.) → c'est ce que fait TFTP par exemple.
     */
    while ((n = recv(fd, buf, BUF_SIZE, 0)) > 0) {
        fwrite(buf, 1, n, fichier_sortie);
        total_octets += n;
        printf("[PID %d] Reçu %d octets (total : %ld)...\n",
               getpid(), n, total_octets);
        fflush(stdout);
    }

    if (n == -1) {
        perror("Erreur recv()");
    }

    fclose(fichier_sortie);

    printf("\n[PID %d] Transfert terminé !\n", getpid());
    printf("[PID %d] Total reçu   : %ld octets\n", getpid(), total_octets);
    printf("[PID %d] Fichier sauvé : %s\n", getpid(), nom_fichier);

    close(fd);
    return EXIT_SUCCESS;
}
