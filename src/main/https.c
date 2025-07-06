#include "./core/common.h"
#include "./https.h"
#include "./main/util.h"
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <fcntl.h>

void https_send_established(ClientState *state){
    const char *response = "HTTP/1.1 200 Connection Established\r\n\r\n";
    send(state->client_fd, response, strlen(response), 0);
}

int https_connect_to_target(ClientState *state) {
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

    https_send_established(state);

    return 1;
}

int receive_data(int fd, char *buffer, size_t capacity) {
    ssize_t received = recv(fd, buffer, capacity, 0);
    if (received == 0) {
        printf("[recv] fd=%d: connection closed by peer\n", fd);
        return -2;
    }
    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            printf("[recv] fd=%d: would block (EAGAIN)\n", fd);
            return 0;
        }
        perror("[recv] error");
        return -1;
    }

    printf("[recv] fd=%d: raw TLS data %zd bytes (not printed)\n", fd, received);
    return received;
}

int send_data(int fd, const char *buffer, size_t length, size_t *sent_offset) {
    while (*sent_offset < length) {
        ssize_t s = send(fd, buffer + *sent_offset, length - *sent_offset, 0);
        if (s < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                printf("[send] fd=%d: would block (EAGAIN), sent %zu/%zu bytes so far\n",
                       fd, *sent_offset, length);
                return 1;
            }
            perror("[send] error");
            return -1;
        }
        printf("[send] fd=%d: sent %zd bytes (total %zu/%zu)\n", fd, s, *sent_offset + s, length);
        *sent_offset += s;
    }
    return 0;
}

int forward_client_to_target(ClientState *state) {
    if (state->request_sent_already < state->request_len) {
        printf("[forward] Resuming sending client -> target, sent=%zu, len=%zu\n",
               state->request_sent_already, state->request_len);
        return send_data(state->target_fd, state->request_buffer,
                         state->request_len, &state->request_sent_already);
    }

    printf("[forward] Reading from client_fd=%d\n", state->client_fd);
    int received = receive_data(state->client_fd,
                                state->request_buffer, state->request_capacity);
    if (received <= 0) return received;

    state->request_len = received;
    state->request_sent_already = 0;

    printf("[forward] Forwarding %d bytes from client -> target_fd=%d\n",
           received, state->target_fd);

    return send_data(state->target_fd, state->request_buffer,
                     state->request_len, &state->request_sent_already);
}

int forward_target_to_client(ClientState *state) {
    if (state->response_sent_already < state->response_len) {
        printf("[forward] Resuming sending target -> client, sent=%zu, len=%zu\n",
               state->response_sent_already, state->response_len);
        return send_data(state->client_fd, state->response_buffer,
                         state->response_len, &state->response_sent_already);
    }

    printf("[forward] Reading from target_fd=%d\n", state->target_fd);
    int received = receive_data(state->target_fd,
                                state->response_buffer, state->response_capacity);
    if (received <= 0) return received;

    state->response_len = received;
    state->response_sent_already = 0;

    printf("[forward] Forwarding %d bytes from target -> client_fd=%d\n",
           received, state->client_fd);

    return send_data(state->client_fd, state->response_buffer,
                     state->response_len, &state->response_sent_already);
}

int https_forward(ClientState *state) {
    fd_set read_fds;
    FD_ZERO(&read_fds);
    FD_SET(state->client_fd, &read_fds);
    FD_SET(state->target_fd, &read_fds);

    int maxfd = (state->client_fd > state->target_fd) ? state->client_fd : state->target_fd;

    struct timeval tv = {0, 0};
    int ret = select(maxfd + 1, &read_fds, NULL, NULL, &tv);
    if (ret < 0) {
        perror("[select] error");
        return -1;
    }

    int status;

    if (FD_ISSET(state->client_fd, &read_fds)) {
        printf("[https_forward] client_fd %d is ready to read\n", state->client_fd);
        status = forward_client_to_target(state);
        if (status == -1 || status == -2) {
            printf("[https_forward] Error forwarding client -> target, status=%d\n", status);
            return -1;
        }
    }

    if (FD_ISSET(state->target_fd, &read_fds)) {
        printf("[https_forward] target_fd %d is ready to read\n", state->target_fd);
        status = forward_target_to_client(state);
        if (status == -1 || status == -2) {
            printf("[https_forward] Error forwarding target -> client, status=%d\n", status);
            return -1;
        }
    }

    return 0;
}