#ifndef MAIN_EPOLL_UTIL_H
#define MAIN_EPOLL_UTIL_H

#include "./main/client.h"
#include <stdint.h>

typedef enum {
    EPOLL_FD_LISTENER,
    EPOLL_FD_CLIENT,
    EPOLL_FD_TARGET,
    EPOLL_FD_TIMER
} EpollFDType;

typedef struct {
    ClientState *client_state;
    EpollFDType fd_type;
    int timer_fd;
} EpollData;

int epoll_add_fd(int epoll_fd, int fd, uint32_t events, EpollData *data);
int epoll_mod_fd(int epoll_fd, int fd, uint32_t events, EpollData *data);
int epoll_del_fd(int epoll_fd, int fd);

#endif