#include "./core/logger.h"
#include "./main/epoll_util.h"
#include "./client.h"
#include "./db/db.h"
#include "./proxy.h"
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>

#define CLIENT_TIMEOUT_SEC 60

void handle_listener_event(int epoll_fd, int server_sock) {
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
            close(client_sock);
            continue;
        }

        int timer_fd = create_timer_fd(CLIENT_TIMEOUT_SEC);
        if (timer_fd == -1) {
            free_client_state(state, epoll_fd);
            continue;
        }

        EpollData *client_data = malloc(sizeof(EpollData));
        EpollData *timer_data = malloc(sizeof(EpollData));
        if (!client_data || !timer_data) {
            free(client_data);
            free(timer_data);
            close(timer_fd);
            free_client_state(state, epoll_fd);
            continue;
        }

        client_data->fd_type = EPOLL_FD_CLIENT;
        client_data->client_state = state;
        client_data->timer_fd = timer_fd;

        timer_data->fd_type = EPOLL_FD_TIMER;
        timer_data->client_state = state;
        timer_data->timer_fd = timer_fd;

        if (epoll_add_fd(epoll_fd, client_sock, EPOLLIN, client_data) == -1) {
            free(client_data);
            free(timer_data);
            close(timer_fd);
            free_client_state(state, epoll_fd);
            continue;
        }

        if (epoll_add_fd(epoll_fd, timer_fd, EPOLLIN, timer_data) == -1) {
            epoll_del_fd(epoll_fd, client_sock);
            free(client_data);
            free(timer_data);
            close(timer_fd);
            free_client_state(state, epoll_fd);
            continue;
        }

        log_debug("Accepted client fd=%d", client_sock);
    }
}

void handle_timer_event(int epoll_fd, EpollData *data) {
    log_debug("Client timed out");

    uint64_t expirations;
    if (read(data->timer_fd, &expirations, sizeof(expirations)) == -1) {
        log_error("Failed to read timerfd: %s", strerror(errno));
    }

    if (data->client_state) {
        free_client_state(data->client_state, epoll_fd);
    }

    epoll_del_fd(epoll_fd, data->timer_fd);
    close(data->timer_fd);
    free(data);
}

void handle_client_event(int epoll_fd, EpollData *data, DB *db) {
    if (!(data->fd_type == EPOLL_FD_CLIENT || data->fd_type == EPOLL_FD_TARGET)) {
        log_error("Invalid fd_type %d", data->fd_type);
        return;
    }
    handle_client(epoll_fd, data, db);

    if (data->client_state->state == CLOSED) {
        epoll_del_fd(epoll_fd, data->timer_fd);
        close(data->timer_fd);
        free(data);
        log_debug("Closed connection");
    }
}