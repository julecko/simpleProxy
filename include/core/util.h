#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>

int parse_host_port(const char *request, char *host, int *port);
char *base64_encode(const unsigned char *input, size_t len);
char *base64_decode(const char* input, size_t len);
const char *get_program_name(const char* path);
char *get_password(const char *prompt);

#endif