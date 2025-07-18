#include "client.h"
#include "./core/logger.h"
#include "./main/util.h"
#include "./main/epoll_util.h"
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>
#include <netdb.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/select.h>
#include <arpa/inet.h>
#include <sys/epoll.h>

// Used only by HTTPS
void send_https_established(int client_fd) {
    const char *response = "HTTP/1.1 200 Connection Established\r\n\r\n";
    send(client_fd, response, strlen(response), 0);
}

int connection_connect(ClientState *state) {
    struct hostent *he = gethostbyname(state->host);
    if (!he) {
        log_error("Could not resolve host: %s", state->host);
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
            return 0; // still connecting
        } else {
            log_error("connect: %s", strerror(errno));
            close(state->target_fd);
            state->target_fd = -1;
            return -1;
        }
    }

    if (state->is_https) {
        send_https_established(state->client_fd);
        state->request_len = 0;
    }

    return 1;
}

static ssize_t recv_into_buffer(int fd, char *buffer, size_t *len, size_t capacity) {
    ssize_t n = recv(fd, buffer + *len, capacity - *len, 0);
    if (n > 0) {
        *len += n;
        return n;
    } else if (n == 0) {
        // Connection closed by peer
        return -2;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 0; // Not an error
    } else {
        return -1; // Real error
    }
}

static int send_from_buffer(int fd, char *buffer, size_t *len) {
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

int connection_forward(int epoll_fd, struct epoll_event event) {
    EpollData *data = (EpollData *)event.data.ptr;
    ClientState *state = data->client_state;
    int fd = event.data.fd;
    uint32_t ev = event.events;

    bool is_client = (data->fd_type == EPOLL_FD_CLIENT);
    bool is_target = (data->fd_type == EPOLL_FD_TARGET);
    bool modified = false;

    // CLIENT → TARGET
    if (is_client && (ev & EPOLLIN)) {
        ssize_t r = recv_into_buffer(state->client_fd, state->request_buffer, &state->request_len, state->request_capacity);
        if (r < 0) return -1;
        modified = true;
    }

    if (is_target && (ev & EPOLLOUT) && state->request_len > 0) {
        ssize_t s = send_from_buffer(state->target_fd, state->request_buffer, &state->request_len);
        if (s < 0) return -1;
        modified = true;
    }

    // TARGET → CLIENT
    if (is_target && (ev & EPOLLIN)) {
        ssize_t r = recv_into_buffer(state->target_fd, state->response_buffer, &state->response_len, state->response_capacity);
        if (r < 0) return -1;
        modified = true;
    }

    if (is_client && (ev & EPOLLOUT) && state->response_len > 0) {
        ssize_t s = send_from_buffer(state->client_fd, state->response_buffer, &state->response_len);
        if (s < 0) return -1;
        modified = true;
    }

    return 0;
}