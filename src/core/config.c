#include "./config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_LINE 256

void trim_newline(char *str) {
    size_t len = strlen(str);
    if (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r'))
        str[len-1] = '\0';
}

Config create_default_config() {
    Config c = {0, 0, NULL, NULL, NULL, NULL};
    return c;
}

void free_config(Config *config) {
    if (!config) return;
    free(config->db_database);
    free(config->db_user);
    free(config->db_pass);
    free(config->db_host);
    config->db_database = NULL;
    config->db_user = NULL;
    config->db_pass = NULL;
    config->db_host = NULL;
}

bool check_config(const Config *config) {
    return (!config->db_database || !config->db_user || !config->db_pass || !config->db_host || !config->db_port || !config->port);
}

Config load_config(const char* filename) {
    Config config = create_default_config();

    FILE *file = fopen(filename, "r");
    if (!file) {
        perror("Failed to open config file");
        return config;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), file)) {
        trim_newline(line);

        if (line[0] == '\0' || line[0] == '#')
            continue;

        char *equal_sign = strchr(line, '=');
        if (!equal_sign) {
            fprintf(stderr, "Invalid line (no '=' found): %s\n", line);
            continue;
        }

        *equal_sign = '\0';
        char *key = line;
        char *value = equal_sign + 1;

        while (isspace((unsigned char)*key)) key++;
        while (isspace((unsigned char)*value)) value++;

        char *end = key + strlen(key) - 1;
        while (end > key && isspace((unsigned char)*end)) *end-- = '\0';

        end = value + strlen(value) - 1;
        while (end > value && isspace((unsigned char)*end)) *end-- = '\0';

        if (strcmp(key, "PORT") == 0) {
            config.port = atoi(value);
        } else if (strcmp(key, "DB_HOST") == 0) {
            free(config.db_host);
            config.db_host = strdup(value);
        } else if (strcmp(key, "DB_DATABASE") == 0) {
            free(config.db_database);
            config.db_database = strdup(value);
        } else if (strcmp(key, "DB_PORT") == 0) {
            config.db_port = atoi(value);
        } else if (strcmp(key, "DB_USER") == 0) {
            free(config.db_user);
            config.db_user = strdup(value);
        }else if (strcmp(key, "DB_PASS") == 0) {
            free(config.db_pass);
            config.db_pass = strdup(value);
        } else {
            fprintf(stderr, "Unknown config key: %s\n", key);
        }
    }

    fclose(file);
    return config;
}

void save_config(const char *filename, Config *config) {
    FILE *file = fopen(filename, "w");
    if (!file) {
        perror("Failed to open config file for writing");
        return;
    }

    fprintf(file, "# SimpleProxy config file\n");
    fprintf(file, "PORT=%d\n", config->port);
    fprintf(file, "DB_DATABASE=%s\n", config->db_database ? config->db_database : "");
    fprintf(file, "DB_USER=%s\n", config->db_user ? config->db_user : "");
    fprintf(file, "DB_PASS=%s\n", config->db_pass ? config->db_pass : "");

    fclose(file);
}