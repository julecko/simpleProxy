#ifndef MAIN_UTIL_H
#define MAIN_UTIL_H

#include <sys/types.h>

void set_nonblocking(int fd);
int parse_host_port(const char *request, char *host, int *port);
int send_all(int fd, const char *buffer, size_t length);
ssize_t recv_into_buffer(int fd, char *buffer, size_t *len, size_t capacity);
int send_from_buffer(int fd, char *buffer, size_t *len);

#endif