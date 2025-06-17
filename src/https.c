#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <sys/select.h>
#include <unistd.h>
#include <string.h>

void handle_https_tunnel(int client_socket, char *target_host, int target_port)
{
    printf("Establishing HTTPS tunnel to %s:%d\n", target_host, target_port);

    int target_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (target_socket < 0)
    {
        perror("socket");
        return;
    }

    struct hostent *he = gethostbyname(target_host);
    if (!he)
    {
        fprintf(stderr, "Could not resolve host: %s\n", target_host);
        close(target_socket);
        return;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(target_port);
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(target_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0){
        perror("connect to target failed");
        close(target_socket);
        return;
    }

    const char *response = "HTTP/1.1 200 Connection Established\r\n\r\n";
    send(client_socket, response, strlen(response), 0);

    fd_set fds;
    char buffer[4096];
    int max_fd = (client_socket > target_socket ? client_socket : target_socket) + 1;

    struct timeval timeout;

    while (1)
    {
        FD_ZERO(&fds);
        FD_SET(client_socket, &fds);
        FD_SET(target_socket, &fds);

        timeout.tv_sec = 5;
        timeout.tv_usec = 0;
        int activity = select(max_fd, &fds, NULL, NULL, &timeout);

        if (activity < 0){
            perror("select");
            break;
        }else if (activity == 0){
            printf("Timeout, closing tunnel\n");
            break;
        }

        if (FD_ISSET(client_socket, &fds)){
            ssize_t n = recv(client_socket, buffer, sizeof(buffer), 0);
            if (n <= 0)break;
            send(target_socket, buffer, n, 0);
        }

        if (FD_ISSET(target_socket, &fds)){
            ssize_t n = recv(target_socket, buffer, sizeof(buffer), 0);
            if (n <= 0) break;
            send(client_socket, buffer, n, 0);
        }
    }
    printf("Request closed\n");
    close(target_socket);
}
