#include "./config.h"
#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <ctype.h>
#include <stdbool.h>

#define MAX_LINE 512

void trim_newline(char *str) {
    size_t len = strlen(str);
    while (len > 0 && (str[len-1] == '\n' || str[len-1] == '\r')) {
        str[--len] = '\0';
    }
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
    return config->db_database && config->db_user && config->db_pass &&
           config->db_host && config->db_port && config->port;
}

void prepare_line(char *line){
    char *src = line;
    char *dst = line;

    while (*src) {
        if (*src == '#'){
            break;
        }
        if (!isspace(*src)) {
            *dst++ = *src;
        }
        src++;
    }
    *dst = '\0';
}

Config load_config(const char *filename) {
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

        prepare_line(line);
        
        char *equal_sign = strchr(line, '=');
        if (!equal_sign) {
            fprintf(stderr, "Invalid line (no '=' found): %s\n", line);
            continue;
        }

        *equal_sign = '\0';
        char *key = line;
        char *value = equal_sign + 1;

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
    fprintf(file, "DB_HOST=%s\n", config->db_host ? config->db_host : "");

    fclose(file);
}

bool update_config_file(const char *filename, const Config *config) {
    char temp_filename[MAX_LINE];
    snprintf(temp_filename, sizeof(temp_filename), "%s.tmp", filename);

    FILE *fin = fopen(filename, "r");
    if (!fin) {
        perror("Failed to open config file for reading");
        return false;
    }

    FILE *fout = fopen(temp_filename, "w");
    if (!fout) {
        perror("Failed to open temp config file for writing");
        fclose(fin);
        return false;
    }

    char line[MAX_LINE];
    while (fgets(line, sizeof(line), fin)) {
        char original_line[MAX_LINE];
        strncpy(original_line, line, MAX_LINE);

        trim_newline(line);
        prepare_line(line);

        char *equal_sign = strchr(line, '=');
        if (!equal_sign) {
            fputs(original_line, fout);
            continue;
        }

        *equal_sign = '\0';
        char *key = line;
        char *value = equal_sign + 1;

        if (strcmp(key, "PORT") == 0) {
            fprintf(fout, "PORT=%d\n", config->port);
        } else if (strcmp(key, "DB_PORT") == 0) {
            fprintf(fout, "DB_PORT=%d\n", config->db_port);
        } else if (strcmp(key, "DB_HOST") == 0) {
            fprintf(fout, "DB_HOST=%s\n", config->db_host ? config->db_host : "");
        } else if (strcmp(key, "DB_DATABASE") == 0) {
            fprintf(fout, "DB_DATABASE=%s\n", config->db_database ? config->db_database : "");
        } else if (strcmp(key, "DB_USER") == 0) {
            fprintf(fout, "DB_USER=%s\n", config->db_user ? config->db_user : "");
        } else if (strcmp(key, "DB_PASS") == 0) {
            fprintf(fout, "DB_PASS=%s\n", config->db_pass ? config->db_pass : "");
        } else {
            fputs(original_line, fout);
        }
    }

    fclose(fin);
    fclose(fout);

    if (remove(filename) != 0) {
        perror("Failed to delete original config file");
        return false;
    }
    if (rename(temp_filename, filename) != 0) {
        perror("Failed to rename temp file to config file");
        return false;
    }

    return true;
}
