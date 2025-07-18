#include "./core/common.h"
#include "./core/logger.h"
#include "./main/timeout.h"
#include <sys/timerfd.h>
#include <errno.h>
#include <unistd.h>
#include <string.h>
#include <time.h>

int create_timer_fd(int timeout_sec) {
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (tfd == -1) {
        log_error("timerfd_create failed: %s", strerror(errno));
        return -1;
    }

    const struct itimerspec its = {
        .it_value = { .tv_sec = timeout_sec, .tv_nsec = 0 },
        .it_interval = { 0, 0 }
    };


    if (timerfd_settime(tfd, 0, &its, NULL) == -1) {
        log_error("timerfd_settime failed: %s", strerror(errno));
        close(tfd);
        return -1;
    }

    return tfd;
}

void reset_timer(int timer_fd, int timeout_sec) {
    if (timer_fd == -1) return;

    struct itimerspec its;
    its.it_value.tv_sec = TIMEOUT_SEC;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    if (timerfd_settime(timer_fd, 0, &its, NULL) == -1) {
        log_error("Failed to reset timer: %s", strerror(errno));
    }
}