#ifndef MAIN_UTIL_H
#define MAIN_UTIL_H

void set_nonblocking(int fd);
int parse_host_port(const char *request, char *host, int *port);

#endif