#ifndef CONFIG_H
#define CONFIG_H

#include <stdbool.h>

typedef struct Config {
    int port;
    int db_port;
    char* db_host;
    char* db_database;
    char* db_user;
    char* db_pass;
} Config;

Config create_default_config();
bool check_config(const Config *config);
Config load_config(const char *filename);
bool update_config_file(const char *filename, const Config *config);

#endif