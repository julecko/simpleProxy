#ifndef MAIN_HTTP_H
#define MAIN_HTTP_H

#include <stddef.h>

int forward_request(int client_sock, const char *host, int port, const char *request, size_t request_len);

#endif