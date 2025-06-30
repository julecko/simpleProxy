#include <arpa/inet.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>

int forward_request(int client_sock, const char *host, int port, const char *request, size_t request_len) {
    struct hostent *he = gethostbyname(host);
    if (!he) {
        fprintf(stderr, "Could not resolve host: %s\n", host);
        return -1;
    }

    int server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("socket");
        return -1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(port);
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect");
        close(server_sock);
        return -1;
    }

    ssize_t sent = 0;
    while (sent < (ssize_t)request_len) {
        ssize_t s = send(server_sock, request + sent, request_len - sent, 0);
        if (s <= 0) {
            perror("send to server");
            close(server_sock);
            return -1;
        }
        sent += s;
    }

    char buffer[4096];
    ssize_t n;
    while ((n = recv(server_sock, buffer, sizeof(buffer), 0)) > 0) {
        ssize_t sent_to_client = 0;
        while (sent_to_client < n) {
            ssize_t s = send(client_sock, buffer + sent_to_client, n - sent_to_client, 0);
            if (s <= 0) break;
            sent_to_client += s;
        }
        if (sent_to_client < n) break;
    }

    close(server_sock);
    return 0;
}