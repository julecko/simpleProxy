#ifndef MAIN_TIMEOUT_H
#define MAIN_TIMEOUT_H

int create_timer_fd(int timeout_sec);
void reset_client_timer(int timer_fd);

#endif