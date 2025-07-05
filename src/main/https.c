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

int https_forward(ClientState *state) {
    char buffer[4096];
    ssize_t n;

    n = recv(state->client_fd, buffer, sizeof(buffer), 0);
    if (n > 0) {
        send(state->target_fd, buffer, n, 0);
    } else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
        return -1;
    }

    n = recv(state->target_fd, buffer, sizeof(buffer), 0);
    if (n > 0) {
        send(state->client_fd, buffer, n, 0);
    } else if (n == 0 || (n < 0 && errno != EAGAIN && errno != EWOULDBLOCK)) {
        return -1;
    }

    return 0;
}
