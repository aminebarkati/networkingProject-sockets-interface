#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#define PORT 8080

int main(void)
{
    /* Create a UDP socket (IPv4 + datagram semantics). */
    int fd = socket(AF_INET, SOCK_DGRAM, 0);
    if (fd < 0) { perror("socket"); exit(1); }

    /* Bind to all interfaces on PORT so clients can send datagrams here. */
    struct sockaddr_in addr = {
        .sin_family      = AF_INET,
        .sin_addr.s_addr = INADDR_ANY,
        .sin_port        = htons(PORT),
    };

    if (bind(fd, (struct sockaddr *)&addr, sizeof(addr)) < 0) {
        perror("bind"); exit(1);
    }

    printf("Serveur UDP en attente sur le port %d...\n", PORT);

    while (1) {
        char buf[256];
        struct sockaddr_in client_addr;
        socklen_t client_len = sizeof(client_addr);

        /* Wait for one UDP packet and capture sender address. */
        int n = recvfrom(fd, buf, sizeof(buf) - 1, 0,
                         (struct sockaddr *)&client_addr, &client_len);
        if (n <= 0) continue;
        buf[n] = '\0';

        printf("Client %s:%d dit : %s",
               inet_ntoa(client_addr.sin_addr),
               ntohs(client_addr.sin_port), buf);

        /* Reply to the same client once per second for 60 seconds. */
        for (int i = 0; i < 60; i++) {
            time_t t = time(NULL);
            char msg[64];
            strftime(msg, sizeof(msg), "Il est %H:%M:%S !\n", localtime(&t));
            /* sendto needs the destination every time in UDP. */
            sendto(fd, msg, strlen(msg), 0,
                   (struct sockaddr *)&client_addr, client_len);
            sleep(1);
        }

        sendto(fd, "Au revoir\n", 10, 0,
               (struct sockaddr *)&client_addr, client_len);
    }

    close(fd);
    return 0;
}
