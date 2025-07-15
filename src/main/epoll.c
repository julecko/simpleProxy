#include "./main/client.h"
#include "./main/epoll.h"
#include <sys/epoll.h>
#include <errno.h>

int epoll_add_fd(int epoll_fd, int fd, uint32_t events, EpollData *data) {
    struct epoll_event ev = {
        .events = events | EPOLLET,
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
        .events = events | EPOLLET,
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