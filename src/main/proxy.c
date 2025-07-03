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

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        perror("setsockopt(SO_REUSEADDR) failed");
        close(server_fd);
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

// === STATE HANDLERS ===

void handle_reading_request(DB *db, ClientState *state) {
    int result = read_request_nonblocking(state);
    if (result < 0) {
        state->state = CLOSING;
    } else if (result == 1) {
        state->state = AUTHENTICATING;
    }
}

void handle_authenticating(DB *db, ClientState *state) {
    if (has_valid_auth(db, state->request_buffer)) {
        printf("Authentication successful\n");
        state->is_https = strncmp(state->request_buffer, "CONNECT ", 8) == 0;

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
}

void handle_connecting(DB *db, ClientState *state) {
    int result = state->is_https
        ? https_connect_to_target(state)
        : http_connect_to_target(state);

    if (result == -1) {
        state->state = CLOSING;
    } else if (result == 1) {
        state->state = FORWARDING;
    }
}

void handle_forwarding(DB *db, ClientState *state) {
    int result = state->is_https
        ? https_forward(state)
        : http_forward(state);
    if (result != 0) {
        state->state = CLOSING;
    }
}

void handle_closing(DB *db, ClientState *state) {
    printf("Closing connection for fd %d\n", state->client_fd);
}

typedef void (*StateHandler)(DB *, ClientState *);

static StateHandler state_handlers[] = {
    [READING_REQUEST] = handle_reading_request,
    [AUTHENTICATING] = handle_authenticating,
    [CONNECTING] = handle_connecting,
    [CONNECTING_TO_TARGET] = handle_connecting,
    [FORWARDING] = handle_forwarding,
    [CLOSING] = handle_closing,
};


void handle_client(DB *db, ClientState *state) {
    if (state->state >= 0 && state->state < sizeof(state_handlers)/sizeof(state_handlers[0])) {
        StateHandler handler = state_handlers[state->state];
        if (handler) {
            handler(db, state);
        } else {
            fprintf(stderr, "[WARN] No handler for state %d\n", state->state);
        }
    } else {
        fprintf(stderr, "[ERROR] Invalid state %d\n", state->state);
    }
}
