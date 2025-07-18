#include "./main/client.h"
#include "./core/logger.h"
#include "./main/epoll_util.h"
#include "./main/timeout.h"
#include "./main/util.h"
#include <sys/epoll.h>
#include <errno.h>
#include <string.h>
#include <stdbool.h>
#include <unistd.h>
#include <stdlib.h>


int epoll_add_fd(int epoll_fd, int fd, uint32_t events, EpollData *data) {
    struct epoll_event ev = {
        .events = events,
        .data.ptr = data
    };
    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, fd, &ev) == -1) {
        log_error("epoll_ctl ADD fd=%d failed: %s", fd, strerror(errno));
        return -1;
    }
    return 0;
}

int epoll_mod_fd(int epoll_fd, int fd, uint32_t events, EpollData *data) {
    struct epoll_event ev = {
        .events = events,
        .data.ptr = data
    };
    if (epoll_ctl(epoll_fd, EPOLL_CTL_MOD, fd, &ev) == -1) {
        log_error("epoll_ctl MOD fd=%d failed: %s", fd, strerror(errno));
        return -1;
    }
    return 0;
}

int epoll_del_fd(int epoll_fd, int fd) {
    if (epoll_ctl(epoll_fd, EPOLL_CTL_DEL, fd, NULL) == -1) {
        log_error("epoll_ctl DEL fd=%d failed: %s", fd, strerror(errno));
        return -1;
    }

    return 0;
}

EpollData *epoll_create_data(EpollFDType type, ClientState *state, int timer_fd) {
    EpollData *data = malloc(sizeof(EpollData));
    if (!data) return NULL;

    data->fd_type = type;
    data->client_state = state;
    data->timer_fd = timer_fd;
    return data;
}

bool epoll_register_client_with_timer(int epoll_fd, int client_fd, uint32_t events) {
    set_nonblocking(client_fd);

    ClientState *state = create_client_state(client_fd, 0);
    if (!state) {
        close(client_fd);
        return false;
    }

    int timer_fd = create_timer_fd(TIMEOUT_SEC);
    if (timer_fd == -1) {
        free_client_state(state, epoll_fd);
        return false;
    }

    EpollData *client_data = epoll_create_data(EPOLL_FD_CLIENT, state, timer_fd);
    EpollData *timer_data  = epoll_create_data(EPOLL_FD_TIMER, state, timer_fd);

    if (!client_data || !timer_data) {
        free(client_data);
        free(timer_data);
        close(timer_fd);
        free_client_state(state, epoll_fd);
        return false;
    }

    if (epoll_add_fd(epoll_fd, client_fd, events, client_data) == -1) {
        free(client_data);
        free(timer_data);
        close(timer_fd);
        free_client_state(state, epoll_fd);
        return false;
    }

    if (epoll_add_fd(epoll_fd, timer_fd, EPOLLIN | EPOLLET, timer_data) == -1) {
        epoll_del_fd(epoll_fd, client_fd);
        free(client_data);
        free(timer_data);
        close(timer_fd);
        free_client_state(state, epoll_fd);
        return false;
    }

    log_debug("Accepted client fd=%d", client_fd);
    return true;
}