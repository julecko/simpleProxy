#include "./core/common.h"
#include "./core/logger.h"
#include "./main/timeout.h"
#include "./main/epoll_util.h"
#include "./main/client.h"
#include <sys/timerfd.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>
#include <sys/epoll.h>
#include <stdlib.h>


int create_timer_fd(int timeout_sec) {
    int timer_fd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (timer_fd == -1) {
        perror("timerfd_create");
        return -1;
    }

    struct itimerspec timer_spec;
    memset(&timer_spec, 0, sizeof(timer_spec));
    
    timer_spec.it_interval.tv_sec = timeout_sec;
    timer_spec.it_value.tv_sec = timeout_sec;

    if (timerfd_settime(timer_fd, 0, &timer_spec, NULL) == -1) {
        perror("timerfd_settime");
        close(timer_fd);
        return -1;
    }

    return timer_fd;
}

int register_timer(int epoll_fd, int timeout) {
    int timer_fd = create_timer_fd(timeout);
    if (timer_fd == -1) {
        fprintf(stderr, "Failed to create timerfd\n");
        return -1;
    }
    
    EpollData *timer_data = epoll_create_data(EPOLL_FD_TIMER, NULL);
    if (timer_data == NULL) {
        return -1;
    }

    struct epoll_event ev = {
        .events = EPOLLIN,
        .data.ptr = timer_data
    };

    if (epoll_ctl(epoll_fd, EPOLL_CTL_ADD, timer_fd, &ev) == -1) {
        free(timer_data);
        return -1;
    }

    return timer_fd;
}