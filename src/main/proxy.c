#include "./proxy.h"
#include "./http.h"
#include "./https.h"
#include "./auth.h"
#include "./main/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>

int create_server_socket(int port, int backlog) {
    int server_fd;
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, backlog) < 0) {
        perror("listen failed");
        close(server_fd);
        return -1;
    }

    return server_fd;
}

int accept_connection(int server_sock) {
    struct sockaddr_in client_addr;
    socklen_t addr_len = sizeof(client_addr);
    return accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
}

static int read_request_nonblocking(ClientState *state) {
    if (state->request_len == state->request_capacity) {
        size_t new_capacity = state->request_capacity * 2;
        char *newbuf = realloc(state->request_buffer, new_capacity);
        if (!newbuf) {
            perror("realloc failed");
            return -1;
        }
        state->request_buffer = newbuf;
        state->request_capacity = new_capacity;
    }

    ssize_t n = recv(state->client_fd, state->request_buffer + state->request_len, 
                     state->request_capacity - state->request_len, 0);
    if (n < 0) {
        if (errno == EAGAIN || errno == EWOULDBLOCK) {
            return 0;
        }
        perror("recv failed");
        return -1;
    }
    if (n == 0) {
        if (state->waiting_for_auth_retry) {
            if (time(NULL) > state->auth_retry_deadline) {
                printf("Auth retry timed out\n");
                return -1;
            }
            return 0;
        }

        if (state->request_len == 0) {
            printf("Client closed connection\n");
            return -1;
        }
        return 0;
    }

    state->request_len += n;
    state->request_buffer[state->request_len] = '\0';

    if (strstr(state->request_buffer, "\r\n\r\n") != NULL) {
        return 1;
    }
    return 0;
}

void handle_client(DB *db, ClientState *state) {
    switch (state->state) {
        case READING_REQUEST: {
            int result = read_request_nonblocking(state);
            if (result < 0) {
                state->state = CLOSING;
                return;
            }
            if (result == 1) {
                state->state = AUTHENTICATING;
            }
            break;
        }
        case AUTHENTICATING: {
            if (has_valid_auth(db, state->request_buffer)) {
                printf("Authentication successful\n");
                if (strncmp(state->request_buffer, "CONNECT ", 8) == 0) {
                    state->is_https = 1;
                }
                if (parse_host_port(state->request_buffer, state->host, &state->port) != 0) {
                    printf("Failed to parse Host header\n");
                    state->state = CLOSING;
                    return;
                }
                printf("Connecting to host=%s port=%d\n", state->host, state->port);
                state->state = CONNECTING_TO_TARGET;
            } else {
                printf("Authentication failed or missing\n");
                send_proxy_auth_required(state->client_fd);

                state->state = CLOSING;
            }
            break;
        }
        case CONNECTING_TO_TARGET: {
            if (state->is_https) {
                if (https_connect_to_target(state) != 0) {
                    state->state = CLOSING;
                    return;
                }
            } else {
                if (http_connect_to_target(state) != 0) {
                    state->state = CLOSING;
                    return;
                }
            }
            break;
        }
        case FORWARDING: {
            if (state->is_https) {
                if (https_forward(state) != 0) {
                    state->state = CLOSING;
                    return;
                }
            } else {
                if (http_forward(state) != 0) {
                    state->state = CLOSING;
                    return;
                }
            }
            break;
        }
        case CLOSING:
            // cleanup elsewhere
            break;
    }
}
