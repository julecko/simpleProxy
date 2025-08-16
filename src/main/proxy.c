#include "./proxy.h"
#include "./core/logger.h"
#include "./main/timeout.h"
#include "./connection.h"
#include "./auth.h"
#include "./main/util.h"
#include "./main/epoll_util.h"
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
#include <sys/epoll.h>

// Forward declaration
static void handle_reading_request(int epoll_fd, struct epoll_event event, DB *db);
static void handle_authenticating(int epoll_fd, struct epoll_event event, DB *db);
static void handle_initialize_connection(int epoll_fd, struct epoll_event event, DB *db);
static void handle_connecting(int epoll_fd, struct epoll_event event, DB *db);
static void handle_forwarding(int epoll_fd, struct epoll_event event, DB *db);
static void handle_closing(int epoll_fd, struct epoll_event event, DB *db);

static bool register_forwarding_fds(int epoll_fd, ClientState *state) {
    EpollData *client_data = epoll_create_data(EPOLL_FD_CLIENT, state);
    if (!client_data) {
        log_error("Failed to allocate EpollData for client");
        return false;
    }

    struct epoll_event client_ev = {
        .events = EPOLLIN | EPOLLOUT,
        .data.ptr = client_data
    };

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, state->client_fd, &client_ev) == -1) {
        log_error("epoll_ctl MOD client_fd failed: %s", strerror(errno));
        free(client_data);
        return false;
    }

    EpollData *target_data = epoll_create_data(EPOLL_FD_TARGET, state);
    if (!target_data) {
        log_error("Failed to allocate EpollData for target");
        return false;
    }

    struct epoll_event target_ev = {
        .events = EPOLLIN | EPOLLOUT,
        .data.ptr = target_data
    };

    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, state->target_fd, &target_ev) == -1) {
        log_error("epoll_ctl MOD target_fd failed: %s", strerror(errno));
        free(target_data);
        return false;
    }

    return true;
}


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

static void handle_reading_request(int epoll_fd, struct epoll_event event, DB *db) {
    EpollData *data = (EpollData *)event.data.ptr;
    ClientState *state = data->client_state;

    int result = read_request_nonblocking(state);

    if (result < 0) {
        data->client_state->state = CLOSING;
        handle_client(epoll_fd, event, db);
    } else if (result == 1) {
        data->client_state->state = AUTHENTICATING;
        handle_client(epoll_fd, event, db);
    }
}

static void handle_authenticating(int epoll_fd, struct epoll_event event, DB *db) {
    EpollData *data = (EpollData *)event.data.ptr;
    ClientState *state = data->client_state;

    if (has_valid_auth(db, state->request_buffer)) {
        log_debug("Authentication successful");
        state->is_https = strncmp(state->request_buffer, "CONNECT ", 8) == 0;

        if (parse_host_port(state->request_buffer, state->host, &state->port) != 0) {
            log_warn("Failed to parse Host header");
            state->state = CLOSING;
            return;
        }

        log_debug("Connecting to host=%s port=%d", state->host, state->port);
        state->state = INITIALIZE_CONNECTION;
        handle_client(epoll_fd, event, db);
    } else {
        log_debug("Authentication failed or missing");
        send_proxy_auth_required(state->client_fd);
        state->state = CLOSING;
        handle_client(epoll_fd, event, db);
    }
}

static void handle_initialize_connection(int epoll_fd, struct epoll_event event, DB *db) {
    EpollData *data = (EpollData *)event.data.ptr;
    ClientState *state = data->client_state;

    int result = connection_connect(state);

    if (result == -1) {
        state->state = CLOSING;
        return;
    }

    struct epoll_event ev = {0};
    ev.events = EPOLLOUT;
    ev.data.ptr = epoll_create_data(EPOLL_FD_TARGET, state);

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, state->target_fd, &ev) < 0) {
        log_error("epoll_ctl ADD target_fd failed: %s", strerror(errno));
        close(state->target_fd);
        state->state = CLOSING;
        return;
    }

    state->state = CONNECTING;
}

static void handle_connecting(int epoll_fd, struct epoll_event event, DB *db) {
    EpollData *data = (EpollData *)event.data.ptr;
    ClientState *state = data->client_state;

    int err = 0;
    socklen_t len = sizeof(err);

    if (getsockopt(state->target_fd, SOL_SOCKET, SO_ERROR, &err, &len) < 0 || err != 0) {
        log_error("connect failed: %s", strerror(err ? err : errno));
        close(state->target_fd);
        state->target_fd = -1;
        state->state = CLOSING;
        handle_client(epoll_fd, event, db);
        return;
    }

    if (state->is_https) {
        send_https_established(state->client_fd);
        state->request_len = 0;
    }

    state->response_len = 0;
    state->state = FORWARDING;

    register_forwarding_fds(epoll_fd, state);
}

static void handle_forwarding(int epoll_fd, struct epoll_event event, DB *db) {
    EpollData *data = (EpollData *)event.data.ptr;
    int result = connection_forward(epoll_fd, event);
    if (result != 0) {
        data->client_state->state = CLOSING;
    }
}

static void handle_closing(int epoll_fd, struct epoll_event event, DB *db) {
    EpollData *data = (EpollData *)event.data.ptr;
    ClientState *state = data->client_state;

    state->expired = true;

    if (state->client_fd != -1) {
        epoll_del_fd(epoll_fd, state->client_fd);
        close(state->client_fd);
        state->client_fd = -1;
    }
    if (state->target_fd != -1) {
        epoll_del_fd(epoll_fd, state->target_fd);
        close(state->target_fd);
        state->target_fd = -1;
    }

    free(event.data.ptr);
    event.data.ptr = NULL;
}

typedef void (*StateHandler)(int epoll_fd, struct epoll_event event, DB *db);

static StateHandler state_handlers[] = {
    [READING_REQUEST] = handle_reading_request,
    [AUTHENTICATING] = handle_authenticating,
    [INITIALIZE_CONNECTION] = handle_initialize_connection,
    [CONNECTING] = handle_connecting,
    [FORWARDING] = handle_forwarding,
    [CLOSING] = handle_closing,
};

void handle_client(int epoll_fd, struct epoll_event event, DB *db) {
    EpollData *data = (EpollData *)event.data.ptr;
    if (data->client_state->expired == true) {
        return;
    }

    if (data->client_state->state >= 0 && data->client_state->state < sizeof(state_handlers)/sizeof(state_handlers[0])) {
        StateHandler handler = state_handlers[data->client_state->state];
        if (handler) {
            handler(epoll_fd, event, db);
        } else {
            log_error("No handler for state %d, closing connection", data->client_state->state);
            data->client_state->state = CLOSING;
            handle_client(epoll_fd, event, db);
        }
    } else if (data->client_state->state == (sizeof(state_handlers)/sizeof(state_handlers[0]))) {
        return;
    } else {
        log_error("Invalid state %d, closing connection", data->client_state->state);
        data->client_state->state = CLOSING;
    }
}