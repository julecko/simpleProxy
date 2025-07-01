#ifndef SETUP_H
#define SETUP_H

#include <stddef.h>

int ask_port(char *buffer, size_t buffer_size);
char *ask_db_host(char *buffer, size_t buffer_size);
int ask_db_port(char *buffer, size_t buffer_size);
char *ask_db_database(char *buffer, size_t buffer_size);
char *ask_db_user(char *buffer, size_t buffer_size);
char *ask_db_pass(char *buffer, size_t buffer_size);
int start_enable_service();
int prompt_and_enable_service();

#endif