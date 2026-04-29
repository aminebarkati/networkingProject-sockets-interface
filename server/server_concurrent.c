/*
 * =============================================================================
 * server_concurrent.c — Serveur TCP concurrent multi-services
 * =============================================================================
 *
 * Partie 5 du TP — Serveur en mode concurrent
 *
 * Description :
 *   Serveur concurrent capable de traiter plusieurs clients simultanément
 *   sur trois services distincts, chacun écoutant sur un port dédié.
 *
 *   Architecture (deux niveaux de fork) :
 *     Niveau 1 — fork par service au démarrage :
 *       main() fork → processus écouteur Service 2 (port 8081)
 *       main() fork → processus écouteur Service 3 (port 8082)
 *       main()      → processus écouteur Service 1 (port 8080)
 *
 *     Niveau 2 — fork par client à chaque accept() :
 *       Écouteur → accept() → fork() → fils gère le client
 *                           → père retourne immédiatement à accept()
 *
 *   Services :
 *     Service 1 (port 8080) : envoie l'heure courante 60 fois (sleep 1s)
 *     Service 2 (port 8081) : envoie le nombre de processus (ps aux | wc -l)
 *     Service 3 (port 8082) : transfère le contenu de shared_file.txt
 *
 *   Gestion des zombies :
 *     signal(SIGCHLD, SIG_IGN) demande au noyau de nettoyer automatiquement
 *     les processus fils terminés, sans waitpid() explicite.
 *
 * Usage :
 *   Compilation : make server_concurrent
 *   Exécution   : ./server_concurrent
 *   Test service 1 : nc localhost 8080  (taper "Bonjour" + Entrée)
 *   Test service 2 : nc localhost 8081
 *   Test service 3 : nc localhost 8082
 *   Test parallèle : cd ../Client && ./test_parallel.sh
 *
 * Ports : 8080 (service 1), 8081 (service 2), 8082 (service 3)
 * Protocole : TCP (SOCK_STREAM)
 * =============================================================================
 */

#include <stdio.h>       /* printf, perror, fgets, popen, pclose, fopen, fread */
#include <stdlib.h>      /* exit                                               */
#include <string.h>      /* strlen, snprintf                                   */
#include <unistd.h>      /* close, sleep, fork                                 */
#include <time.h>        /* time, localtime, strftime                          */
#include <signal.h>      /* signal, SIGCHLD, SIG_IGN                          */
#include <sys/socket.h>  /* socket, bind, listen, accept, send                 */
#include <sys/wait.h>    /* waitpid (inclus pour portabilité)                  */
#include <netinet/in.h>  /* struct sockaddr_in, htons, INADDR_ANY              */
#include <arpa/inet.h>   /* inet_ntoa                                          */

#define PORT_S1   8080              /* port du service 1 (heure)               */
#define PORT_S2   8081              /* port du service 2 (processus)           */
#define PORT_S3   8082              /* port du service 3 (fichier)             */
#define BACKLOG   10               /* taille de la file d'attente des connexions*/
#define FILE_PATH "shared_file.txt" /* fichier servi par le service 3          */

/* =========================================================================== */
/* make_server : crée une socket TCP, la configure et la met en écoute         */
/* Factorisation : les 3 services font la même chose, seul le port change      */
/* Retourne le descripteur de socket prêt à recevoir des accept()              */
/* =========================================================================== */
static int make_server(int port)
{
    /* Crée une socket TCP (même qu'en server_tcp.c)                         */
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    /* Permet de relancer le serveur sans attendre la fin du TIME_WAIT TCP   */
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons(port),  /* port différent pour chaque service */
    };

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(fd, BACKLOG) < 0) { perror("listen"); exit(1); }

    return fd;  /* retourne le descripteur pour qu'on puisse appeler accept() */
}

/* =========================================================================== */
/* service1 : envoie l'heure 60 fois au client (port 8080)                    */
/* Identique à handle_client de server_tcp.c                                   */
/* =========================================================================== */
static void service1(int client_fd)
{
    char buf[256];
    /* Attend le "Bonjour" du client avant de commencer                      */
    int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return;  /* client déconnecté avant d'envoyer quoi que ce soit*/
    buf[n] = '\0';
    printf("[S1] Client dit : %s", buf);

    for (int i = 0; i < 60; i++) {
        time_t t = time(NULL);
        char msg[64];
        strftime(msg, sizeof(msg), "Il est %H:%M:%S !\n", localtime(&t));
        if (send(client_fd, msg, strlen(msg), 0) < 0) break;  /* client parti */
        sleep(1);
    }
    send(client_fd, "Au revoir\n", 10, 0);
}

/* =========================================================================== */
/* service2 : renvoie le nombre de processus en cours sur le serveur          */
/* =========================================================================== */
static void service2(int client_fd)
{
    char result[256] = {0};  /* = {0} initialise tout le tableau à zéro      */

    /* popen() exécute une commande shell et retourne un FILE* pour lire     */
    /* sa sortie standard, comme si on lisait un fichier                     */
    /* "ps aux" : liste tous les processus de la machine                     */
    /* "wc -l"  : compte le nombre de lignes → nombre de processus          */
    /* "r"      : mode lecture                                               */
    FILE *f = popen("ps aux | wc -l", "r");
    if (f) {
        /* fgets lit une ligne depuis le FILE* retourné par popen()          */
        fgets(result, sizeof(result), f);
        /* pclose() ferme le processus ouvert par popen() (important !)      */
        pclose(f);
    }

    char msg[300];
    /* snprintf : formate une chaîne dans msg, sans déborder (taille limitée)*/
    int len = snprintf(msg, sizeof(msg), "Nombre de processus : %s", result);
    send(client_fd, msg, len, 0);
}

/* =========================================================================== */
/* service3 : transfère le contenu de shared_file.txt au client               */
/* =========================================================================== */
static void service3(int client_fd)
{
    /* fopen() ouvre un fichier et retourne un FILE* (pointeur de fichier)   */
    /* "r" = mode lecture seule                                              */
    FILE *f = fopen(FILE_PATH, "r");
    if (!f) {
        /* Fichier absent : prévenir le client et sortir                     */
        const char *err = "Fichier introuvable\n";
        send(client_fd, err, strlen(err), 0);
        return;
    }

    char buf[1024];  /* buffer de lecture : on lit par blocs de 1024 octets  */
    size_t n;        /* nombre d'octets réellement lus par fread()           */

    /* fread() lit jusqu'à sizeof(buf) octets depuis le fichier              */
    /* retourne le nombre d'octets lus (peut être < sizeof(buf) à la fin)   */
    /* la boucle continue jusqu'à ce que fread() retourne 0 (fin du fichier)*/
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        send(client_fd, buf, n, 0);  /* envoie le bloc lu au client          */

    fclose(f);  /* ferme le fichier (libère les ressources)                  */
}

/* =========================================================================== */
/* run_listener : boucle d'écoute générique avec fork par client              */
/* server_fd  : socket serveur qui appelle accept()                           */
/* handler    : pointeur de fonction → service1, service2 ou service3         */
/*              void (*handler)(int) = "fonction qui prend un int, rien renvoie"*/
/* name       : nom du service pour les messages de log                      */
/* =========================================================================== */
static void run_listener(int server_fd,
                         void (*handler)(int),
                         const char *name)
{
    /* signal() installe un gestionnaire pour un signal donné               */
    /* SIGCHLD  : signal envoyé au père quand un fils se termine            */
    /* SIG_IGN  : "ignore ce signal"                                        */
    /* Effet    : le noyau Linux nettoie automatiquement les fils terminés  */
    /*            sans qu'on ait besoin d'appeler waitpid()                 */
    /*            Sinon les fils deviennent des "zombies" dans la table PID */
    signal(SIGCHLD, SIG_IGN);

    printf("[%s] en écoute...\n", name);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        /* accept() bloque jusqu'à une nouvelle connexion                   */
        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr,
                               &client_len);
        if (client_fd < 0) { perror("accept"); continue; }

        printf("[%s] connexion de %s:%d\n", name,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        /* fork() duplique le processus courant                             */
        /* À partir d'ici, DEUX processus exécutent le même code           */
        /* La seule différence : la valeur retournée par fork()             */
        /*   Dans le fils  → fork() retourne 0                             */
        /*   Dans le père  → fork() retourne le PID du fils (entier > 0)   */
        /*   Si erreur     → fork() retourne -1                            */
        pid_t pid = fork();
        if (pid < 0) { perror("fork"); close(client_fd); continue; }

        if (pid == 0) {
            /* ── CODE EXÉCUTÉ UNIQUEMENT PAR LE FILS ── */

            /* Le fils n'a pas besoin de la socket d'écoute                 */
            /* Si on ne la ferme pas, elle reste ouverte dans les deux      */
            /* processus ce qui peut causer des problèmes à l'arrêt        */
            close(server_fd);

            /* Appelle le bon service (service1, service2 ou service3)      */
            /* handler est un pointeur de fonction, (*handler) l'appelle    */
            handler(client_fd);

            close(client_fd);  /* ferme la connexion avec le client         */
            exit(0);           /* le fils se termine proprement             */
        }

        /* ── CODE EXÉCUTÉ UNIQUEMENT PAR LE PÈRE ── */

        /* Le père n'a plus besoin de client_fd : le fils s'en occupe      */
        /* Si on ne le ferme pas ici, le descripteur reste ouvert dans     */
        /* le père et la connexion ne se fermera jamais côté client        */
        close(client_fd);

        /* Le père retourne immédiatement à accept() pour le client suivant*/
        /* Il peut donc accepter N clients simultanément                   */
    }
}

/* =========================================================================== */
/* main : crée les 3 sockets, fork 2 fois pour les services 2 et 3,           */
/*        puis gère le service 1 directement                                   */
/* =========================================================================== */
int main(void)
{
    /* Crée les 3 sockets d'écoute avant tout fork                          */
    /* Elles sont créées ici pour être héritées par les processus fils      */
    int fd1 = make_server(PORT_S1);  /* socket pour service 1 (port 8080)   */
    int fd2 = make_server(PORT_S2);  /* socket pour service 2 (port 8081)   */
    int fd3 = make_server(PORT_S3);  /* socket pour service 3 (port 8082)   */

    /* ── Premier fork : crée le processus écouteur pour le service 2 ── */
    if (fork() == 0) {
        /* On est dans le fils (fork() == 0)                                */
        /* Ce fils ne doit gérer QUE le service 2, donc on ferme fd1, fd3  */
        /* pour ne pas garder des descripteurs ouverts inutilement          */
        close(fd1); close(fd3);
        run_listener(fd2, service2, "Service2");
        exit(0);  /* ne devrait pas être atteint (run_listener boucle infini)*/
    }

    /* ── Deuxième fork : crée le processus écouteur pour le service 3 ── */
    if (fork() == 0) {
        /* On est dans le deuxième fils                                      */
        close(fd1); close(fd2);
        run_listener(fd3, service3, "Service3");
        exit(0);
    }

    /* ── Le processus principal (père) gère le service 1 ── */
    /* Il ferme fd2 et fd3 dont ses fils s'occupent déjà                    */
    close(fd2); close(fd3);
    run_listener(fd1, service1, "Service1");

    return 0;  /* jamais atteint                                             */
}
