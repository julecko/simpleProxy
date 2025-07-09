#ifndef MAIN_UTIL_H
#define MAIN_UTIL_H

#include <sys/types.h>

void set_nonblocking(int fd);
int parse_host_port(const char *request, char *host, int *port);

#endif