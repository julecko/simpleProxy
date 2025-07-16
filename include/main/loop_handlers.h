#ifndef MAIN_LOOP_HANDLERS_H
#define MAIN_LOOP_HANDLERS_H

#include "./epoll_util.h"
#include "./db/db.h"

void handle_listener_event(int epoll_fd, int server_sock);
void handle_timer_event(int epoll_fd, EpollData *data);
void handle_client_event(int epoll_fd, EpollData *data, DB *db);

#endif