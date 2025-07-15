#include "./proxy.h"
#include "./core/logger.h"
#include "./connection.h"
#include "./auth.h"
#include "./main/util.h"
#include "./main/epoll.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <fcntl.h>
#include <sys/select.h>

int create_server_socket(int port, int backlog) {
    int server_fd;
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        log_error("socket failed: %s", strerror(errno));
        return -1;
    }

    int opt = 1;
    if (setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) < 0) {
        log_error("setsockopt(SO_REUSEADDR) failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(port);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        log_error("bind failed: %s", strerror(errno));
        close(server_fd);
        return -1;
    }

    if (listen(server_fd, backlog) < 0) {
        log_error("listen failed: %s", strerror(errno));
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
    if (state->request_len == state->request_capacity - 1) {
        size_t new_capacity = state->request_capacity * 2;
        char *newbuf = realloc(state->request_buffer, new_capacity);
        if (!newbuf) {
            log_error("realloc failed: %s", strerror(errno));
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
        log_error("recv failed: %s", strerror(errno));
        return -1;
    }
    if (n == 0) {
        if (state->request_len == 0) {
            log_debug("Client closed connection");
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

void handle_reading_request(int epoll_fd, EpollData *data, DB *db) {
    ClientState *state = data->client_state;
    int result = read_request_nonblocking(state);
    if (result < 0) {
        data->client_state->state = CLOSING;
    } else if (result == 1) {
        data->client_state->state = AUTHENTICATING;
    }
}

void handle_authenticating(int epoll_fd, EpollData *data, DB *db) {
    ClientState *state = data->client_state;
    if (has_valid_auth(db, state->request_buffer)) {
        log_debug("Authentication successful");
        state->is_https = strncmp(state->request_buffer, "CONNECT ", 8) == 0;

        if (parse_host_port(state->request_buffer, state->host, &state->port) != 0) {
            log_warn("Failed to parse Host header");
            state->state = CLOSING;
            return;
        }

        log_debug("Connecting to host=%s port=%d on slot=%d", state->host, state->port, state->slot);
        state->state = INITIALIZE_CONNECTION;
    } else {
        log_debug("Authentication failed or missing");
        send_proxy_auth_required(state->client_fd);
        state->state = CLOSING;
    }
}

void handle_initialize_connection(int epoll_fd, EpollData *data, DB *db) {
    ClientState *state = data->client_state;
    int result = connection_connect(state);

    switch (result) {
    case -1:
        state->state = CLOSING;
        break;
    case 0:
        state->state = CONNECTING;
        break;
    case 1:
        state->response_len = 0;
        state->state = FORWARDING;
        break;
    default:
        log_error("Undefined result from handle initializing of connection");
        break;
    }
}

void handle_connecting(int epoll_fd, EpollData *data, DB *db) {
    ClientState *state = data->client_state;
    int err = 0;
    socklen_t len = sizeof(err);

    // Check if the socket is writable using non-blocking send
    ssize_t test = send(state->target_fd, NULL, 0, 0);
    if (test == -1 && (errno == EAGAIN || errno == EWOULDBLOCK)) {
        // Still connecting
        return;
    }

    if (getsockopt(state->target_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
        log_error("connect failed: %s", strerror(err ? err : errno));
        close(state->target_fd);
        state->target_fd = -1;
        state->state = CLOSING;
    } else {
        if (state->is_https) {
            send_https_established(state->client_fd);
            state->request_len = 0;
        }
        state->response_len = 0;
        state->state = FORWARDING;
    }
}

void handle_forwarding(int epoll_fd, EpollData *data, DB *db) {
    ClientState *state = data->client_state;
    int result = connection_forward(state);
    if (result != 0) {
        state->state = CLOSING;
    }
}

void handle_closing(int epoll_fd, EpollData *data, DB *db) {
    ClientState *state = data->client_state;
    free_client_state(state);
    state->state = CLOSED;
}

typedef void (*StateHandler)(int epoll_fd, EpollData *data, DB *db);

static StateHandler state_handlers[] = {
    [READING_REQUEST] = handle_reading_request,
    [AUTHENTICATING] = handle_authenticating,
    [INITIALIZE_CONNECTION] = handle_initialize_connection,
    [CONNECTING] = handle_connecting,
    [FORWARDING] = handle_forwarding,
    [CLOSING] = handle_closing,
};

void handle_client(int epoll_fd, EpollData *data, DB *db) {
    if (data->client_state->state >= 0 && data->client_state->state < sizeof(state_handlers)/sizeof(state_handlers[0])) {
        StateHandler handler = state_handlers[data->client_state->state];
        if (handler) {
            handler(epoll_fd, data, db);
        } else {
            log_error("No handler for state %d, closing connection", data->client_state->state);
            data->client_state->state = CLOSING;
        }
    } else {
        log_error("Invalid state %d, closing connection", data->client_state->state);
        data->client_state->state = CLOSING;
    }
}