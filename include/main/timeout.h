#ifndef MAIN_TIMEOUT_H
#define MAIN_TIMEOUT_H

#define TIMEOUT_SEC 10

int create_timer_fd(int timeout_sec);
void reset_timer(int timer_fd, int timeout_sec);

#endif