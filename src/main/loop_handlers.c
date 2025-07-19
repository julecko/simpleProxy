#include "./core/logger.h"
#include "./main/timeout.h"
#include "./main/util.h"
#include "./main/epoll_util.h"
#include "./client.h"
#include "./db/db.h"
#include "./proxy.h"
#include <errno.h>
#include <sys/epoll.h>
#include <unistd.h>
#include <string.h>

void handle_listener_event(int epoll_fd, int server_sock) {
    while (1) {
        int client_sock = accept_connection(server_sock);
        if (client_sock == -1) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            log_error("accept failed: %s", strerror(errno));
            break;
        }

        epoll_register_client(epoll_fd, client_sock, EPOLLIN);
    }
}

void handle_timer_event(int epoll_fd, EpollData *data) {
    // ADD
}

void handle_client_event(int epoll_fd, struct epoll_event event, DB *db) {
    EpollData *data = (EpollData *)event.data.ptr;
    if (!(data->fd_type == EPOLL_FD_CLIENT || data->fd_type == EPOLL_FD_TARGET)) {
        log_error("Invalid fd_type %d", data->fd_type);
        return;
    }

    handle_client(epoll_fd, event, db);
}