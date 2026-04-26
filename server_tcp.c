#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT    8080
#define BACKLOG 10

static void handle_client(int client_fd, struct sockaddr_in *client_addr)
{
    char buf[256];
    int n;

    printf("Connexion de %s:%d\n",
           inet_ntoa(client_addr->sin_addr),
           ntohs(client_addr->sin_port));

    n = recv(client_fd, buf, sizeof(buf) - 1, 0);
    if (n <= 0) return;
    buf[n] = '\0';
    printf("Client dit : %s", buf);

    for (int i = 0; i < 60; i++) {
        time_t t = time(NULL);
        char msg[64];
        strftime(msg, sizeof(msg), "Il est %H:%M:%S !\n", localtime(&t));
        if (send(client_fd, msg, strlen(msg), 0) < 0) break;
        sleep(1);
    }

    send(client_fd, "Au revoir\n", 10, 0);
}

int main(void)
{
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd < 0) { perror("socket"); exit(1); }

    int opt = 1;
    setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons(PORT),
    };

    if (bind(server_fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }
    if (listen(server_fd, BACKLOG) < 0) { perror("listen"); exit(1); }

    printf("Serveur TCP en attente sur le port %d...\n", PORT);

    while (1) {
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        int client_fd = accept(server_fd, (struct sockaddr *)&client_addr, &client_len);
        if (client_fd < 0) { perror("accept"); continue; }

        handle_client(client_fd, &client_addr);
        close(client_fd);
    }

    close(server_fd);
    return 0;
}
