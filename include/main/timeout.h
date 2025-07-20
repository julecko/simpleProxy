#ifndef MAIN_TIMEOUT_H
#define MAIN_TIMEOUT_H

#include <stdbool.h>

int create_timer_fd(int timeout_sec);
int register_timer(int epoll_fd, int timeout);

#endif