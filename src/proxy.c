#include "util.h"
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

int read_request(int sock, char **headers_out, size_t *len_out) {
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

int parse_host_port(const char *request, char *host, int *port) {
    const char *host_header = strstr(request, "\nHost:");
    if (!host_header)
        host_header = strstr(request, "\nhost:");
    if (!host_header) {
        fprintf(stderr, "No Host header found\n");
        return -1;
    }

    host_header += 6;
    while (*host_header == ' ' || *host_header == '\t') host_header++;

    const char *end = strchr(host_header, '\r');
    if (!end) end = strchr(host_header, '\n');
    if (!end) end = host_header + strlen(host_header);

    size_t len = end - host_header;
    char host_port[256];
    if (len >= sizeof(host_port)) len = sizeof(host_port) - 1;
    strncpy(host_port, host_header, len);
    host_port[len] = 0;

    char *colon = strchr(host_port, ':');
    if (colon) {
        *colon = 0;
        *port = atoi(colon + 1);
    } else {
        *port = 80;
    }

    strcpy(host, host_port);
    return 0;
}

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

void handle_client(int client_socket) {
    char *request = NULL;
    size_t request_len = 0;

    if (read_request(client_socket, &request, &request_len) != 0) {
        printf("Failed to read client request headers\n");
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

    forward_request(client_socket, host, port, request, request_len);

    free(request);
    close(client_socket);
}
