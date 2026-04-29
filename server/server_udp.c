/*
 * =============================================================================
 * server_udp.c — Serveur UDP (mode non connecté)
 * =============================================================================
 *
 * Partie 4 du TP — Transfert de messages en mode non connecté
 *
 * Description :
 *   Serveur UDP qui attend un datagramme initial d'un client, puis lui
 *   envoie l'heure courante 60 fois à raison d'un datagramme par seconde.
 *   Protocole applicatif :
 *     1. Le client envoie "Bonjour\n" (recvfrom récupère son adresse)
 *     2. Le serveur envoie l'heure via sendto() 60 fois avec sleep(1)
 *     3. Le serveur envoie "Au revoir\n"
 *
 *   DIFFÉRENCES CLÉS vs TCP :
 *     - Pas de listen() ni accept() : UDP n'établit pas de connexion
 *     - recvfrom() et sendto() remplacent recv() et send()
 *       car l'adresse du client doit être précisée à chaque envoi
 *     - 1 sendto() = 1 datagramme = 1 recvfrom() côté client (pas de fusion)
 *     - Les datagrammes peuvent être perdus ou arriver dans le désordre
 *     - Le serveur ne peut répondre qu'à un client à la fois (séquentiel)
 *
 * Usage :
 *   Compilation : make server_udp   (ou gcc -o server_udp server_udp.c)
 *   Exécution   : ./server_udp
 *   Test rapide : echo "Bonjour" | nc -u localhost 8080
 *
 * Ports : 8080
 * Protocole : UDP (SOCK_DGRAM, mode non connecté, non fiable, non ordonné)
 * =============================================================================
 */

#include <stdio.h>       /* printf, perror                                    */
#include <stdlib.h>      /* exit                                              */
#include <string.h>      /* strlen                                            */
#include <unistd.h>      /* close, sleep                                      */
#include <time.h>        /* time, localtime, strftime                         */
#include <sys/socket.h>  /* socket, bind, sendto, recvfrom                    */
#include <netinet/in.h>  /* struct sockaddr_in, htons, INADDR_ANY             */
#include <arpa/inet.h>   /* inet_ntoa                                         */

#define PORT 8080   /* port d'écoute du serveur                              */

int main(void)
{
    /* SOCK_DGRAM : type UDP — datagrammes indépendants, sans connexion      */
    /* Chaque appel recvfrom/sendto est un paquet réseau autonome            */
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }

    /* Même structure d'adresse qu'en TCP pour le bind                       */
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,  /* écoute sur toutes les interfaces  */
        .sin_port        = htons(PORT), /* port en big-endian (format réseau) */
    };

    /* bind() est nécessaire en UDP aussi : il indique sur quel port         */
    /* le serveur va "écouter" les datagrammes entrants                      */
    /* Pas de listen() : UDP n'a pas de notion de connexion à établir        */
    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }

    printf("Serveur UDP en attente sur le port %d...\n", PORT);

    /* Boucle infinie : traite un client, puis attend le suivant             */
    while (1) {
        char buf[256];                          /* buffer pour le message reçu */
        struct sockaddr_in client_addr;         /* adresse de l'expéditeur     */
        socklen_t client_len = sizeof(client_addr);

        /* recvfrom() BLOQUE jusqu'à recevoir un datagramme UDP              */
        /* Arguments : (socket, buffer, taille, flags, adresse, taille_addr) */
        /* CRUCIAL : remplit client_addr avec l'IP+port de l'expéditeur      */
        /* Sans client_addr, on ne saurait pas où envoyer la réponse         */
        int n = recvfrom(fd, buf, sizeof(buf) - 1, 0,
                         (struct sockaddr *)&client_addr, &client_len);
        if (n <= 0) continue;  /* erreur ou datagramme vide : recommence    */
        buf[n] = '\0';         /* termine la chaîne pour printf             */

        printf("Client %s:%d dit : %s",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port), buf);

        /* Envoie l'heure 60 fois au même client                            */
        for (int i = 0; i < 60; i++) {
            time_t t = time(NULL);
            char msg[64];
            strftime(msg, sizeof(msg), "Il est %H:%M:%S !\n", localtime(&t));

            /* sendto() envoie un datagramme VERS client_addr               */
            /* Arguments : (socket, données, taille, flags, dest, taille)   */
            /* En TCP, send() sait déjà où envoyer (connexion établie)      */
            /* En UDP, il faut préciser la destination à CHAQUE envoi       */
            /* car il n'y a pas de connexion persistante                     */
            sendto(fd, msg, strlen(msg), 0,
                   (struct sockaddr *)&client_addr, client_len);

            sleep(1);  /* 1 datagramme par seconde                         */
        }

        /* Dernier datagramme : message de fin                              */
        sendto(fd, "Au revoir\n", 10, 0,
               (struct sockaddr *)&client_addr, client_len);

        /* Retour à recvfrom() : attend un nouveau client                   */
        /* Contrairement à TCP, pas de close() ici — la socket reste ouverte*/
    }

    close(fd);  /* jamais atteint (Ctrl+C)                                  */
    return 0;
}
