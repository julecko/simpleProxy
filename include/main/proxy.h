#ifndef MAIN_PROXY_H
#define MAIN_PROXY_H

#include "./db/db.h"
#include "./client.h"

int create_server_socket(int port, int backlog);
int accept_connection(int server_sock);
void handle_client(DB *db, ClientState *state);

#endif
