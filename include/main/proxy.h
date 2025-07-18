#ifndef MAIN_PROXY_H
#define MAIN_PROXY_H

#include "./db/db.h"
#include "./client.h"
#include "./main/epoll_util.h"

#include <sys/epoll.h>

int create_server_socket(int port, int backlog);
int accept_connection(int server_sock);
void handle_client(int epoll_fd, struct epoll_event event, DB *db);

#endif
