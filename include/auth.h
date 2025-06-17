#ifndef AUTH_H
#define AUTH_H

int has_valid_auth(const char *request);
void send_proxy_auth_required(int client_socket);

#endif