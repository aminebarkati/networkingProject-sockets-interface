#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <signal.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT_S1  8080
#define PORT_S2  8081
#define PORT_S3  8082
#define BACKLOG  10
#define FILE_PATH "shared_file.txt"

/* ── socket helpers ─────────────────────────────────────────────────────── */

static int make_server(int port)
{
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons(port),
    };

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(fd, BACKLOG) < 0) { perror("listen"); exit(1); }
    return fd;
}

/* ── service handlers (each runs in a dedicated child process) ──────────── */

/* Service 1 — heure x60 (port 8080) */
static void service1(int client_fd)
{
    char buf[256];
    int n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return;
    buf[n] = '\0';
    printf("[S1] Client dit : %s", buf);

    for (int i = 0; i < 60; i++) {
        time_t t = time(NULL);
        char msg[64];
        strftime(msg, sizeof(msg), "Il est %H:%M:%S !\n", localtime(&t));
        if (send(client_fd, msg, strlen(msg), 0) < 0) break;
        sleep(1);
    }
    send(client_fd, "Au revoir\n", 10, 0);
}

/* Service 2 — nombre de processus (port 8081) */
static void service2(int client_fd)
{
    char result[256] = {0};
    FILE *f = popen("ps aux | wc -l", "r");
    if (f) {
        fgets(result, sizeof(result), f);
        pclose(f);
    }
    char msg[300];
    int len = snprintf(msg, sizeof(msg), "Nombre de processus : %s", result);
    send(client_fd, msg, len, 0);
}

/* Service 3 — transfert de fichier (port 8082) */
static void service3(int client_fd)
{
    FILE *f = fopen(FILE_PATH, "r");
    if (!f) {
        const char *err = "Fichier introuvable\n";
        send(client_fd, err, strlen(err), 0);
        return;
    }
    char buf[1024];
    size_t n;
    while ((n = fread(buf, 1, sizeof(buf), f)) > 0)
        send(client_fd, buf, n, 0);
    fclose(f);
}

/* ── generic accept loop with fork-per-client ───────────────────────────── */

static void run_listener(int server_fd,
                         void (*handler)(int),
                         const char *name)
{
    signal(SIGCHLD, SIG_IGN); /* auto-reap children */

    printf("[%s] en écoute...\n", name);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd,
                               (struct sockaddr *)&client_addr,
                               &client_len);
        if (client_fd < 0) { perror("accept"); continue; }

        printf("[%s] connexion de %s:%d\n", name,
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port));

        pid_t pid = fork();
        if (pid < 0) { perror("fork"); close(client_fd); continue; }

        if (pid == 0) {         /* child — handles exactly one client */
            close(server_fd);
            handler(client_fd);
            close(client_fd);
            exit(0);
        }

        close(client_fd);       /* parent — back to accept() */
    }
}

/* ── main : fork one listener process per service ───────────────────────── */

int main(void)
{
    int fd1 = make_server(PORT_S1);
    int fd2 = make_server(PORT_S2);
    int fd3 = make_server(PORT_S3);

    /* listener for service 2 */
    if (fork() == 0) {
        close(fd1); close(fd3);
        run_listener(fd2, service2, "Service2");
        exit(0);
    }

    /* listener for service 3 */
    if (fork() == 0) {
        close(fd1); close(fd2);
        run_listener(fd3, service3, "Service3");
        exit(0);
    }

    /* main process handles service 1 */
    close(fd2); close(fd3);
    run_listener(fd1, service1, "Service1");

    return 0;
}
