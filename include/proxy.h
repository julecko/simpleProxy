#ifndef PROXY_H
#define PROXY_H

int create_server_socket(int port, int backlog);
int accept_connection(int server_sock);
void handle_client(int client_sock);

#endif
