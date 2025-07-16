#include "./core/common.h"
#include "./core/logger.h"
#include "./core/util.h"
#include "./proxy.h"
#include "./db/db.h"
#include "./client.h"
#include "./main/util.h"
#include "./main/epoll_util.h"
#include "./loop_handlers.h"
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
int epoll_fd = -1;

void cleanup_resources() {
    if (server_sock != -1) {
        close(server_sock);
        server_sock = -1;
    }

    if (epoll_fd != -1) {
        close(epoll_fd);
        epoll_fd = -1;
    }
}

void cleanup(int signum) {
    ssize_t ignored;
    ignored = write(STDOUT_FILENO, "\nCleaning up and exiting...\n", 28);
    ignored = write(STDOUT_FILENO, "Closing socket\n", 15);
    cleanup_resources();
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

void exit_with_error(const char *msg, const char *err) {
    log_error("%s: %s", msg, err);
    cleanup_resources();
    exit(EXIT_FAILURE);
}

void run_loop(DB *db) {
    set_nonblocking(server_sock);
    epoll_fd = epoll_create1(EPOLL_CLOEXEC);
    if (epoll_fd == -1) {
        exit_with_error("epoll_create1 failed", strerror(errno));
    }

    EpollData *server_data = calloc(1, sizeof(EpollData));
    if (!server_data) {
        exit_with_error("calloc failed", strerror(errno));
    }

    server_data->fd_type = EPOLL_FD_LISTENER;
    server_data->client_state = NULL;
    if (epoll_add_fd(epoll_fd, server_sock, EPOLLIN, server_data) == -1) {
        free(server_data);
        exit_with_error("epoll_add_fd failed", strerror(errno));
    }

    struct epoll_event events[MAX_CONNECTIONS * 3];

    while (1) {
        int n_ready = epoll_wait(epoll_fd, events, MAX_CONNECTIONS * 3, -1);
        if (n_ready == -1) {
            if (errno == EINTR) continue;
            exit_with_error("epoll_wait failed", strerror(errno));
        }

        for (int i = 0; i < n_ready; ++i) {
            EpollData *data = events[i].data.ptr;

            if (data && data->fd_type == EPOLL_FD_LISTENER && data->client_state == NULL) {
                handle_listener_event(epoll_fd, server_sock);
            } else if (data->fd_type == EPOLL_FD_TIMER) {
                handle_timer_event(epoll_fd, data);
            } else {
                handle_client_event(epoll_fd, data, db);
            }
        }
    }

    cleanup_resources();
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
