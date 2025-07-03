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
    if (state->state == CONNECTING) {
        fd_set write_fds;
        FD_ZERO(&write_fds);
        FD_SET(state->target_fd, &write_fds);
        struct timeval tv = {0, 0};

        int ret = select(state->target_fd + 1, NULL, &write_fds, NULL, &tv);
        if (ret > 0 && FD_ISSET(state->target_fd, &write_fds)) {
            int err = 0;
            socklen_t len = sizeof(err);
            if (getsockopt(state->target_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
                fprintf(stderr, "connect failed: %s\n", strerror(err ? err : errno));
                close(state->target_fd);
                state->target_fd = -1;
                state->state = CLOSING;
                return -1;
            }

            return 1;
        }

        return 0;
    }

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

    int flags = fcntl(state->target_fd, F_GETFL, 0);
    if (flags < 0 || fcntl(state->target_fd, F_SETFL, flags | O_NONBLOCK) < 0) {
        perror("fcntl");
        close(state->target_fd);
        state->target_fd = -1;
        return -1;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(state->port);
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(state->target_fd, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        if (errno == EINPROGRESS) {
            state->state = CONNECTING;
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

int http_forward(ClientState *state) {
    ssize_t sent = 0;
    while (sent < (ssize_t)state->request_len) {
        ssize_t s = send(state->target_fd, state->request_buffer + sent, 
                         state->request_len - sent, 0);
        if (s < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            perror("send to server");
            return -1;
        }
        sent += s;
    }

    ssize_t n = recv(state->target_fd, state->response_buffer + state->response_len, 
                     state->response_capacity - state->response_len, 0);
    if (n > 0) {
        state->response_len += n;
        ssize_t sent_to_client = 0;
        while (sent_to_client < (ssize_t)state->response_len) {
            ssize_t s = send(state->client_fd, state->response_buffer + sent_to_client, 
                             state->response_len - sent_to_client, 0);
            if (s < 0) {
                if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                return -1;
            }
            sent_to_client += s;
        }
        if (sent_to_client > 0) {
            memmove(state->response_buffer, state->response_buffer + sent_to_client, 
                    state->response_len - sent_to_client);
            state->response_len -= sent_to_client;
        }
    } else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
        return -1;
    }

    return 0;
}
