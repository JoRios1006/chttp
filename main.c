#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <string.h>
#include <netinet/tcp.h>

int main(void) {
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    int yes = 1;
setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, &yes, sizeof(yes));

    struct sockaddr_in addr = {
        .sin_family = AF_INET,
        .sin_port = htons(8080),
        .sin_addr.s_addr = INADDR_ANY
    };

    bind(fd, (struct sockaddr *)&addr, sizeof(addr));
    listen(fd, 5);

    // Respuesta con formato HTTP/1.1 válido
    const char *response = 
        "HTTP/1.1 200 OK\r\n"
        "Content-Type: text/plain; charset=utf-8\r\n"
        "Content-Length: 21\r\n"
        "Connection: close\r\n"
        "\r\n"                     // Separador obligatorio cabecera/cuerpo
        "Hola mundo, probando\n";

    while (1) {
        int client_fd = accept(fd, NULL, NULL);
        if (client_fd < 0) continue;

        send(client_fd, response, strlen(response), 0);
        close(client_fd);
    }

    return 0;
}
