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
        // connection closed by peer
        return -2;
    }
    if (received < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK)
            return 0; // no data available now
        perror("recv");
        return -1;
    }
    return received;
}

int send_data(int fd, const char *buffer, size_t length) {
    ssize_t sent = 0;
    while (sent < (ssize_t)length) {
        ssize_t s = send(fd, buffer + sent, length - sent, 0);
        if (s < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) {
                // would block now, try later or break
                break;
            }
            perror("send");
            return -1;
        }
        sent += s;
    }
    // If sent < length, not all data sent, caller must handle this
    // For now return 0 if no error, else -1
    return 0;
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
        perror("select");
        return -1;
    }

    int status;

    if (FD_ISSET(state->client_fd, &read_fds)) {
        status = forward_client_to_target(state);
        switch (status){
            case -1:
                perror("HTTPS forwarding failed");
            case -2:
                return -1;
        }
    }

    if (FD_ISSET(state->target_fd, &read_fds)) {
        status = forward_target_to_client(state);
        switch (status){
            case -1:
                perror("HTTPS forwarding failed");
            case -2:
                return -1;
        }
    }

    return 0;
}