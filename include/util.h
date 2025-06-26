#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

int parse_host_port(const char *request, char *host, int *port);
char* base64_encode(const unsigned char *input, size_t len);

#endif