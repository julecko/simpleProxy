#ifndef COMMANDS_H
#define COMMANDS_H

#include "./db/db.h"
#include <stddef.h>

typedef int (*CommandFunc)(int argc, char **argv, DB *db);

typedef struct {
    const char *name;
    const char *description;
    CommandFunc func;
} CommandEntry;

int cmd_migrate(int argc, char **argv, DB *db);
int cmd_drop(int argc, char **argv, DB *db);
int cmd_reset(int argc, char **argv, DB *db);
int cmd_add(int argc, char **argv, DB *db);
int cmd_remove(int argc, char **argv, DB *db);
int cmd_test(int argc, char **argv, DB *db);
void print_command_help(const char *program_name);

extern const CommandEntry COMMANDS[];
extern const size_t NUM_COMMANDS;

#endif
