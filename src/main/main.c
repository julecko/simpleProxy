#include "./core/common.h"
#include "./core/logger.h"
#include "./core/util.h"
#include "./proxy.h"
#include "./db/db.h"
#include "./client.h"
#include "./main/util.h"
#include "./main/epoll.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>
#include <fcntl.h>
#include <sys/epoll.h>

#define MAX_CONNECTIONS 1024
#define BACKLOG 128
#define CLIENT_TIMEOUT_SEC 60

int server_sock = -1;
ClientState *clients[MAX_CONNECTIONS] = {0};

void cleanup(int signum) {
    ssize_t ignored;
    ignored = write(STDOUT_FILENO, "\nCleaning up and exiting...\n", 28);
    ignored = write(STDOUT_FILENO, "Closing socket\n", 15);

    if (server_sock != -1) {
        close(server_sock);
        server_sock = -1;
    }
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (clients[i]) {
            free_client_state(clients[i]);
            clients[i] = NULL;
        }
    }
    _exit(EXIT_SUCCESS);
}

void register_signal_handler(void (*handler)(int)) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM");
    }
}

int find_free_slot() {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!clients[i]) return i;
    }
    return -1;
}

size_t count_free_slots() {
    size_t counter = 0;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!clients[i]) counter++;
    }
    return counter;
}

void run_loop(DB *db) {
    set_nonblocking(server_sock);
    int epoll_fd = epoll_create1(0);
    if (epoll_fd == -1) die("epoll_create1 failed: %s", strerror(errno));

    EpollData *server_data = calloc(1, sizeof(EpollData));
    server_data->fd_type = EPOLL_FD_LISTENER;
    server_data->client_state = NULL;
    epoll_add_fd(epoll_fd, server_sock, EPOLLIN, server_data);

    struct epoll_event events[MAX_EVENTS];

    while (1) {
        int n_ready = epoll_wait(epoll_fd, events, MAX_EVENTS, -1);
        if (n_ready == -1) {
            if (errno == EINTR) continue;
            die("epoll_wait failed: %s", strerror(errno));
        }

        for (int i = 0; i < n_ready; ++i) {
            EpollData *data = events[i].data.ptr;

            if (data && data->fd_type == EPOLL_FD_LISTENER && data->client_state == NULL) {
                while (1) {
                    int client_sock = accept_connection(server_sock);
                    if (client_sock == -1) {
                        if (errno == EAGAIN || errno == EWOULDBLOCK) break;
                        log_error("accept failed: %s", strerror(errno));
                        break;
                    }

                    set_nonblocking(client_sock);
                    ClientState *state = create_client_state(client_sock, 0);

                    if (!state) {
                        log_warn("Failed to allocate client state");
                        close(client_sock);
                        continue;
                    }

                    EpollData *client_data = malloc(sizeof(EpollData));
                    EpollData *timer_data;
                    client_data->fd_type = EPOLL_FD_CLIENT;
                    client_data->client_state = state;
                    client_data->timer_fd = create_timer_fd(CLIENT_TIMEOUT_SEC);

                    if (client_data->timer_fd != -1) {
                        timer_data = malloc(sizeof(EpollData));
                        timer_data->fd_type = EPOLL_FD_TIMER;
                        timer_data->client_state = state;
                        timer_data->timer_fd = client_data->timer_fd;
                    } else {
                        log_error("Timer creation failed");
                        continue;
                    }

                    if (epoll_add_fd(epoll_fd, client_sock, EPOLLIN, client_data) == -1 || epoll_add_fd(epoll_fd, timer_data->timer_fd, EPOLLIN, timer_data) == -1) {
                        free_client_state(state);
                        free(client_data);
                        continue;
                    }

                    log_debug("Accepted client fd=%d", client_sock);
                }
            } else {
                if (data->fd_type == EPOLL_FD_TIMER) {
                    log_debug("Client timed out");

                    if (data->client_state) {
                        if (data->client_state->client_fd != -1)
                            epoll_del_fd(epoll_fd, data->client_state->client_fd);
                        if (data->client_state->target_fd != -1)
                            epoll_del_fd(epoll_fd, data->client_state->target_fd);
                        free_client_state(data->client_state);
                    }

                    epoll_del_fd(epoll_fd, data->timer_fd);
                    close(data->timer_fd);
                    free(data);
                    continue;
                }

                if (!(data->fd_type == EPOLL_FD_CLIENT || data->fd_type == EPOLL_FD_TARGET)) {
                    log_error("Invalid fd_type %d", data->fd_type);
                    continue;
                }
                handle_client(epoll_fd, data, db);

                if (data->client_state->state == CLOSED) {
                    if (data->client_state->client_fd != -1) {
                        epoll_del_fd(epoll_fd, data->client_state->client_fd);
                    }
                    if (data->client_state->target_fd != -1) {
                        epoll_del_fd(epoll_fd, data->client_state->target_fd);
                    }
                    free_client_state(data->client_state);
                    free(data);
                    log_debug("Closed connection");
                }
            }
        }
    }

    close(epoll_fd);
}

int main(int argc, char *argv[]) {
    if (check_print_version(argc, argv)) {
        return EXIT_SUCCESS;
    }

    register_signal_handler(cleanup);

    logger_init(stdout, stderr, LOG_DEBUG, 0);

    Config config = load_config(CONF_PATH);
    if (!check_config(&config)) {
        log_error("Configuration file is incomplete or corrupted\n");
        return EXIT_FAILURE;
    }

    DB db;
    if (!db_create(&db, &config)) {
        log_error("Failed to connect to database.");
        return EXIT_FAILURE;
    }

    server_sock = create_server_socket(config.port, BACKLOG);
    if (server_sock < 0) {
        return EXIT_FAILURE;
    }

    log_info("Proxy server running on port %d...", config.port);
    fflush(stdout);

    run_loop(&db);

    close(server_sock);
    return EXIT_SUCCESS;
}
