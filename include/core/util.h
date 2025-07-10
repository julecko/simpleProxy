#ifndef UTIL_H
#define UTIL_H

#include <stddef.h>
#include <stdbool.h>

char *base64_encode(const unsigned char *input, size_t len);
char *base64_decode(const char *input, size_t len);
const char *get_program_name(const char *path);
char *get_password(const char *prompt);
bool get_input(char *buffer, size_t buffer_len, const char *prompt);
bool is_all_digits(const char *str);
int check_print_version(int argc, char *argv[]);

#endif