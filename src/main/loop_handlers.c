#include "./core/logger.h"
#include "./main/timeout.h"
#include "./main/util.h"
#include "./main/epoll_util.h"
#include "./main/client_registry.h"
#include "./client.h"
#include "./db/db.h"
#include "./proxy.h"
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>

#define CLIENT_TIMEOUT_SEC 25

void handle_listener_event(int epoll_fd, int server_sock) {
    while (1) {
        int client_sock = accept_connection(server_sock);
        if (client_sock == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            log_error("accept failed: %s", strerror(errno));
            break;
        }

        ClientState *state = epoll_register_client(epoll_fd, client_sock, EPOLLIN);
        if (!state) {
            free_client_state(&state, epoll_fd);
            log_warn("State creation failed");
            return;
        }

        int slot = state_registry_add(state);
        if (slot == -1) {
            free_client_state(&state, epoll_fd);
            log_warn("Slot coulnt be founf");
            return;
        }

        state->slot = slot;
    }
}

void timeout_callback(ClientState *state, void *arg) {
    int epoll_fd = *(int *)arg;
    time_t now = time(NULL);

    if (now - state->last_active > 10 && !state->expired) {
        return;
    }
    log_debug("Client expired or timed out");

    if (state->expired) {
        state_registry_remove(state->slot);
        free_client_state(&state, epoll_fd);
    } else {
        if (state->client_fd != -1 && state->target_fd != -1) {
            epoll_del_fd(epoll_fd, state->client_fd);
            epoll_del_fd(epoll_fd, state->target_fd);
        }
        state->expired = true;
    }

}

void handle_timer_event(int epoll_fd, int timer_fd) {
    log_debug("Timer triggered\n");
    uint64_t expirations;
    ssize_t s = read(timer_fd, &expirations, sizeof(expirations));
    if (s != sizeof(expirations)) {
        log_error("Failed to read from timerfd: %s", strerror(errno));
        return;
    }

    state_registry_for_each(timeout_callback, &epoll_fd);
}

void handle_client_event(int epoll_fd, struct epoll_event event, DB *db) {
    EpollData *data = (EpollData *)event.data.ptr;
    if (!(data->fd_type == EPOLL_FD_CLIENT || data->fd_type == EPOLL_FD_TARGET)) {
        log_error("Invalid fd_type %d", data->fd_type);
        return;
    }

    handle_client(epoll_fd, event, db);
}