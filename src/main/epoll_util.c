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
        int err = errno;
        switch (errno) {
            case ENOENT:
                return 0;
        }
        log_error("epoll_ctl DEL fd=%d failed: %s", fd, strerror(err));
        return -1;
    }

    return 0;
}

EpollData *epoll_create_data(EpollFDType type, ClientState *state) {
    EpollData *data = malloc(sizeof(EpollData));
    if (!data) return NULL;

    data->fd_type = type;
    data->client_state = state;
    return data;
}

ClientState *epoll_register_client(int epoll_fd, int client_fd, uint32_t events) {
    set_nonblocking(client_fd);

    ClientState *state = create_client_state(client_fd);
    if (!state) {
        close(client_fd);
        return NULL;
    }

    EpollData *client_data = epoll_create_data(EPOLL_FD_CLIENT, state);

    if (!client_data) {
        free(client_data);
        free_client_state(&state, epoll_fd);
        return NULL;
    }

    if (epoll_add_fd(epoll_fd, client_fd, events, client_data) == -1) {
        free(client_data);
        free_client_state(&state, epoll_fd);
        return NULL;
    }

    log_debug("Accepted client fd=%d", client_fd);
    return state;
}