#include "./http.h"
#include "./main/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>

int http_connect_to_target(ClientState *state) {
    struct hostent *he = gethostbyname(state->host);
    if (!he) {
        fprintf(stderr, "Could not resolve host: %s\n", state->host);
        return -1;
    }

    state->target_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (state->target_fd < 0) {
        perror("socket");
        return -1;
    }

    set_nonblocking(state->target_fd);

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(state->port);
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(state->target_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        if (errno == EINPROGRESS) {
            return 0;
        } else {
            perror("connect");
            close(state->target_fd);
            state->target_fd = -1;
            return -1;
        }
    }

    return 1;
}

int send_all(int fd, const char *buffer, size_t length) {
    ssize_t sent = 0;
    while (sent < (ssize_t)length) {
        ssize_t s = send(fd, buffer + sent, length - sent, 0);
        if (s < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            return -1;
        }
        sent += s;
    }
    return sent;
}

ssize_t recv_into_buffer(int fd, char *buffer, size_t *len, size_t capacity) {
    ssize_t n = recv(fd, buffer + *len, capacity - *len, 0);
    if (n > 0) {
        *len += n;
        return n;
    } else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
        return -1;
    }
    return 0;
}

int send_from_buffer(int fd, char *buffer, size_t *len) {
    ssize_t sent = 0;
    while (sent < (ssize_t)(*len)) {
        ssize_t s = send(fd, buffer + sent, *len - sent, 0);
        if (s < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            return -1;
        }
        sent += s;
    }

    if (sent > 0) {
        memmove(buffer, buffer + sent, *len - sent);
        *len -= sent;
    }

    return 0;
}

int http_forward(ClientState *state) {
    if (send_all(state->target_fd, state->request_buffer, state->request_len) < 0) {
        perror("send to server");
        return -1;
    }

    if (recv_into_buffer(state->target_fd, state->response_buffer, &state->response_len, state->response_capacity) < 0) {
        perror("http receiving failed");
        return -1;
    }

    if (send_from_buffer(state->client_fd, state->response_buffer, &state->response_len) < 0) {
        perror("http sending failed");
        return -1;
    }
    
    return 0;
}
