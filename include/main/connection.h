#ifndef MAIN_CONNECTION_H
#define MAIN_CONNECTION_H

#include "./main/client.h"
#include "./main/epoll_util.h"

void send_https_established(int client_fd);
int connection_connect(ClientState *state);
int connection_forward(int epoll_fd, struct epoll_event event);

#endif