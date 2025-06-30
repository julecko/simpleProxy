#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>

int forward_request(int client_sock, const char *host, int port, const char *request, size_t request_len);

#endif