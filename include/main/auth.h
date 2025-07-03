#ifndef MAIN_AUTH_H
#define MAIN_AUTH_H

#include "./db/user.h"
#include "./db/db.h"

int has_valid_auth(DB *db, const char *request);
void send_proxy_auth_required(int client_socket);

#endif