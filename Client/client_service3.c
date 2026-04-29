/*
 * =============================================================================
 * client_service3.c — Client du Service 3 (transfert de fichier, port 8082)
 * =============================================================================
 *
 * Partie 5 du TP — Client de test pour le serveur concurrent
 *
 * Description :
 *   Se connecte au Service 3 du serveur concurrent (port 8082).
 *   Reçoit le contenu du fichier "shared_file.txt" du serveur et le
 *   sauvegarde localement sous le nom "received_file.txt".
 *
 * Usage :
 *   Compilation : gcc -o client_service3 client_service3.c
 *   Exécution   : ./client_service3
 *   Résultat    : fichier "received_file.txt" créé dans le répertoire courant
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

#define SERVER_IP     "127.0.0.1"
#define SERVER_PORT   8082
#define BUF_SIZE      1024
#define OUTPUT_FILE   "received_file.txt"

static void print_timestamp(const char *label)
{
    time_t t = time(NULL);
    char ts[32];
    strftime(ts, sizeof(ts), "%H:%M:%S", localtime(&t));
    printf("[%s] %s (PID=%d)\n", ts, label, getpid());
}

int main(void)
{
    printf("=== Client Service 3 — Transfert fichier (port %d) ===\n\n", SERVER_PORT);
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

    /* Ouvrir le fichier de sortie en écriture */
    FILE *out = fopen(OUTPUT_FILE, "w");
    if (!out) { perror("fopen"); exit(EXIT_FAILURE); }

    char   buf[BUF_SIZE];
    int    n;
    size_t total = 0;

    printf("Réception du fichier...\n");

    while ((n = recv(fd, buf, sizeof(buf) - 1, 0)) > 0) {
        fwrite(buf, 1, n, out);   /* écrire les octets reçus dans le fichier */
        total += n;
    }

    fclose(out);

    printf("[OK] %zu octets reçus → sauvegardé dans \"%s\"\n", total, OUTPUT_FILE);
    print_timestamp("Déconnexion");

    close(fd);
    return EXIT_SUCCESS;
}
