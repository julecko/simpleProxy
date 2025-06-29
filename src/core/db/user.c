#include "./db/migration.h"
#include "./util.h"
#include <string.h>
#include <stdio.h>

#define TABLE_NAME "users"

char *db_user_migration(){
    Column cols[] = {
        {"id", "INT", 0, "PRIMARY KEY AUTO_INCREMENT"},
        {"username", "VARCHAR", 50, "NOT NULL UNIQUE"},
        {"password", "VARCHAR", 100, "NOT NULL"},
        {"created_at", "TIMESTAMP", 0, "DEFAULT CURRENT_TIMESTAMP"},
        {"updated_at", "TIMESTAMP", 0, "DEFAULT CURRENT_TIMESTAMP ON UPDATE CURRENT_TIMESTAMP"}
    };
    Table users_table = {
        TABLE_NAME,
        cols,
        5
    };

    return create_table_sql(&users_table);
}

char *db_user_add(const char* name, const char* base64_pass) {
    char *query = malloc(1024);
    if (!query) return NULL;

    snprintf(query, 1024,
        "INSERT INTO `%s` (username, password) VALUES ('%s', '%s');",
        TABLE_NAME, name, base64_pass);

    return query;
}

char *db_user_add_encrypt(const char* name, const char* pass){
    char *base64_pass = base64_encode(pass, strlen(pass));
    char *query = db_user_add(name, base64_pass);
    
    free(base64_pass);

    return query;
}

char *db_user_verify(const char* name, const char* pass) {
    char *query = malloc(1024);
    if (!query) {
        return NULL;
    }

    snprintf(query, 1024,
        "SELECT 1 FROM `%s` WHERE username = '%s' AND password = '%s' LIMIT 1;",
        TABLE_NAME, name, pass);

    return query;
}

char *db_user_remove(const char *name){
    char *query = malloc(1024);
    if (!query) return NULL;

    snprintf(query, 1024,
        "DELETE FROM `%s` WHERE username = '%s'",
        TABLE_NAME, name);

    return query;
}