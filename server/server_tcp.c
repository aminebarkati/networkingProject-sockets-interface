/*
 * =============================================================================
 * server_tcp.c — Serveur TCP séquentiel (mode connecté)
 * =============================================================================
 *
 * Partie 3 du TP — Transfert de messages en mode connecté
 *
 * Description :
 *   Serveur TCP qui attend la connexion d'un seul client à la fois.
 *   Protocole applicatif :
 *     1. Le client envoie "Bonjour\n"
 *     2. Le serveur répond avec l'heure courante toutes les secondes (x60)
 *     3. Le serveur envoie "Au revoir\n" et ferme la connexion
 *
 *   LIMITATION : le serveur est séquentiel — un second client devra
 *   attendre 60 secondes dans la file listen() avant d'être accepté.
 *   Cette limitation est résolue dans server_concurrent.c.
 *
 * Usage :
 *   Compilation : make server_tcp   (ou gcc -o server_tcp server_tcp.c)
 *   Exécution   : ./server_tcp
 *   Test rapide : nc localhost 8080  (taper "Bonjour" + Entrée)
 *
 * Ports : 8080
 * Protocole : TCP (SOCK_STREAM, mode connecté, fiable, ordonné)
 * =============================================================================
 */

#include <stdio.h>       /* printf, perror                                    */
#include <stdlib.h>      /* exit                                              */
#include <string.h>      /* strlen                                            */
#include <unistd.h>      /* close, sleep                                      */
#include <time.h>        /* time, localtime, strftime                         */
#include <sys/socket.h>  /* socket, bind, listen, accept, send, recv          */
#include <netinet/in.h>  /* struct sockaddr_in, htons, INADDR_ANY             */
#include <arpa/inet.h>   /* inet_ntoa (convertit IP binaire → texte lisible)  */

#define PORT    8080   /* port d'écoute du serveur                            */
#define BACKLOG 10     /* taille max de la file de connexions en attente      */

/* ------------------------------------------------------------------------- */
/* handle_client : gère UN client du "Bonjour" jusqu'à la fermeture          */
/* client_fd     : descripteur de socket propre à ce client (retourné par    */
/*                 accept). C'est un entier qui représente le "canal" ouvert. */
/* client_addr   : adresse IP + port du client (remplie par accept)          */
/* ------------------------------------------------------------------------- */
static void handle_client(int client_fd, struct sockaddr_in *client_addr)
{
    char buf[256];  /* tableau de caractères : buffer pour stocker ce qu'on reçoit */
    int n;          /* nombre d'octets reçus (retourné par recv)                   */

    /* inet_ntoa  : convertit l'IP binaire (4 octets) en chaîne "127.0.0.1"  */
    /* ntohs      : convertit le numéro de port du format réseau → entier    */
    printf("Connexion de %s:%d\n",
           inet_ntoa(client_addr->sin_addr),
           ntohs(client_addr->sin_port));

    /* recv() lit des données depuis la socket et les stocke dans buf        */
    /* arguments : (socket, buffer, taille_max, flags)                       */
    /* retourne  : nombre d'octets lus, 0 si le client a fermé, -1 si erreur*/
    /* sizeof(buf) - 1 : on réserve une place pour le '\0' terminal         */
    n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return;   /* client déconnecté ou erreur : on abandonne      */
    buf[n] = '\0';        /* termine la chaîne C pour pouvoir utiliser printf */
    printf("Client dit : %s", buf);

    /* Boucle 60 fois avec une pause d'1 seconde entre chaque envoi         */
    for (int i = 0; i < 10; i++) {

        time_t t = time(NULL);  /* time() retourne le nombre de secondes    */
                                /* écoulées depuis le 1er janvier 1970      */

        char msg[64];           /* buffer qui va contenir le message formaté */

        /* strftime : formate une date/heure en texte selon un gabarit       */
        /* %H = heure (00-23), %M = minutes (00-59), %S = secondes (00-59)  */
        /* localtime(&t) convertit le timestamp en struct tm (heure locale)  */
        strftime(msg, sizeof(msg), "Il est %H:%M:%S !\n", localtime(&t));

        /* send() envoie des données sur la socket                           */
        /* arguments : (socket, données, taille, flags)                      */
        /* retourne  : nombre d'octets envoyés, -1 si erreur                */
        /* si < 0 : le client s'est déconnecté → on sort de la boucle      */
        if (send(client_fd, msg, strlen(msg), 0) < 0) break;

        sleep(1);  /* pause d'1 seconde (définie dans <unistd.h>)           */
    }

    /* Dernier message avant fermeture                                       */
    /* 10 = strlen("Au revoir\n"), passé en dur pour éviter un appel inutile */
    send(client_fd, "Au revoir\n", 10, 0);
}

/* ------------------------------------------------------------------------- */
/* main : point d'entrée — crée la socket, l'attache au port, attend        */
/* ------------------------------------------------------------------------- */
int main(void)
{
    /* socket() crée une socket et retourne un descripteur (entier >= 0)    */
    /* AF_INET     : famille d'adresses IPv4                                 */
    /* SOCK_STREAM : type TCP — flux d'octets continu, fiable, ordonné      */
    /* 0           : protocole automatique (TCP pour SOCK_STREAM)            */
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }  /* perror affiche l'erreur système */

    int opt = 1;
    /* setsockopt : configure des options sur la socket                      */
    /* SOL_SOCKET  : niveau "socket générique"                               */
    /* SO_REUSEADDR: permet de réutiliser le port immédiatement après arrêt */
    /*               Sans ça : "Address already in use" pendant ~1 minute   */
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    /* sockaddr_in : structure qui décrit une adresse réseau IPv4            */
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,       /* protocole IPv4                  */
        .sin_addr.s_addr = INADDR_ANY,    /* accepte les connexions venant   */
                                          /* de toutes les interfaces réseau */
        .sin_port        = htons(PORT),   /* htons = "host to network short" */
                                          /* les octets du port sont inversés*/
                                          /* sur x86 (little-endian) vs réseau
                                             (big-endian)                    */
    };

    /* bind() attache la socket à l'adresse et au port définis ci-dessus    */
    /* cast (struct sockaddr *) nécessaire car bind() est générique         */
    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }

    /* listen() passe la socket en mode "écoute passive"                    */
    /* BACKLOG = nombre max de clients pouvant attendre dans la file        */
    /* Si la file est pleine, les nouveaux clients reçoivent ECONNREFUSED   */
    if (listen(server_fd, BACKLOG) < 0) { perror("listen"); exit(1); }

    printf("Serveur TCP en attente sur le port %d...\n", PORT);

    /* Boucle infinie : le serveur tourne jusqu'à Ctrl+C                    */
    while (1) {
        struct sockaddr_in client_addr;               /* sera remplie par accept() */
        socklen_t client_len = sizeof(client_addr);   /* taille de la structure   */

        /* accept() BLOQUE ici jusqu'à ce qu'un client se connecte          */
        /* Quand un client arrive, il retourne un NOUVEAU descripteur        */
        /* spécifique à cette connexion                                      */
        /* server_fd continue d'écouter d'autres connexions                 */
        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) { perror("accept"); continue; }  /* erreur → recommence */

        /* Traite le client (bloque 60 secondes), puis ferme SA socket      */
        handle_client(client_fd, &client_addr);

        /* close() libère le descripteur et envoie FIN au client            */
        /* Déclenche la fermeture TCP (FIN → FIN-ACK)                       */
        close(client_fd);

        /* Retour à accept() : attend le client suivant                     */
    }

    close(server_fd);  /* jamais atteint (Ctrl+C), mais bonne pratique     */
    return 0;
}
