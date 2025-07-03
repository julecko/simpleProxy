#ifndef MAIN_PROXY_H
#define MAIN_PROXY_H

#include "./db/db.h"

int create_server_socket(int port, int backlog);
int accept_connection(int server_sock);
void handle_client(DB *db, int client_sock);

#endif
