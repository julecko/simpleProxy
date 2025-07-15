#include "./core/logger.h"
#include <sys/timerfd.h>
#include <linux/time.h>
#include <errno.h>

#define CLIENT_TIMEOUT_SEC 60

int create_timer_fd(int timeout_sec) {
    int tfd = timerfd_create(CLOCK_MONOTONIC, TFD_NONBLOCK | TFD_CLOEXEC);
    if (tfd == -1) {
        log_error("timerfd_create failed: %s", strerror(errno));
        return -1;
    }

    struct itimerspec its;
    its.it_value.tv_sec = timeout_sec;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    if (timerfd_settime(tfd, 0, &its, NULL) == -1) {
        log_error("timerfd_settime failed: %s", strerror(errno));
        close(tfd);
        return -1;
    }

    return tfd;
}

void reset_client_timer(int timer_fd) {
    if (timer_fd == -1) return;

    struct itimerspec its;
    its.it_value.tv_sec = CLIENT_TIMEOUT_SEC;
    its.it_value.tv_nsec = 0;
    its.it_interval.tv_sec = 0;
    its.it_interval.tv_nsec = 0;

    if (timerfd_settime(timer_fd, 0, &its, NULL) == -1) {
        log_error("Failed to reset timer: %s", strerror(errno));
    }
}