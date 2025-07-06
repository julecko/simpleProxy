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
