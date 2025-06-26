#include "./util.h"
#include "./http.h"
#include "./https.h"
#include "./auth.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>

int create_server_socket(int port, int backlog) {
    int server_fd;
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, backlog) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", port);
    return server_fd;
}

int accept_connection(int server_sock) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    return accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
}

static int read_request(int sock, char **headers_out, size_t *len_out) {
    size_t capacity = 4096;
    size_t len = 0;
    char *buffer = malloc(capacity);
    if (!buffer) return -1;

    while (1) {
        ssize_t n = recv(sock, buffer + len, capacity - len, 0);
        if (n <= 0) {
            free(buffer);
            return -1;
        }
        len += n;
        if (len >= 4 && strstr(buffer, "\r\n\r\n") != NULL) {
            break;
        }
        if (len == capacity) {
            capacity *= 2;
            char *newbuf = realloc(buffer, capacity);
            if (!newbuf) {
                free(buffer);
                return -1;
            }
            buffer = newbuf;
        }
    }

    buffer[len] = 0;
    *headers_out = buffer;
    *len_out = len;
    return 0;
}

void handle_client(int client_socket) {
    char *request = NULL;
    size_t request_len = 0;

    if (read_request(client_socket, &request, &request_len) != 0) {
        printf("Failed to read client request headers\n");
        close(client_socket);
        return;
    }

    if (!has_valid_auth(request)) {
        printf("Authentication failed or missing\n");
        send_proxy_auth_required(client_socket);
        free(request);
        close(client_socket);
        return;
    }
    // Extendable by parsing Content-Length and reading the body.

    char host[256];
    int port = 80;
    if (parse_host_port(request, host, &port) != 0) {
        printf("Failed to parse Host header\n");
        free(request);
        close(client_socket);
        return;
    }
    printf("Forwarding request to host=%s port=%d\n", host, port);

    if (strncmp(request, "CONNECT ", 8) == 0) {
        handle_https_tunnel(client_socket, host, port);
    }else{
        forward_request(client_socket, host, port, request, request_len);
    }

    free(request);
    close(client_socket);
}
