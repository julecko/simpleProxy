#ifndef MAIN_LOOP_HANDLERS_H
#define MAIN_LOOP_HANDLERS_H

#include "./epoll_util.h"
#include "./db/db.h"

#include <stdint.h>
#include <sys/epoll.h>

void handle_listener_event(int epoll_fd, int server_sock);
void handle_timer_event(int epoll_fd, int timer_fd);
void handle_client_event(int epoll_fd, struct epoll_event event, DB *db);

#endif