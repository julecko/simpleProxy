#include "./db/migration.h"
#include "./util.h"
#include <string.h>
#include <stdio.h>

#define TABLE_NAME "users"

char *db_user_migration(){
    Column cols[] = {
        {"id", "INT", 0, "PRIMARY KEY AUTO_INCREMENT"},
        {"username", "VARCHAR", 50, "NOT NULL"},
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

char *db_user_add(const char* name, const char* pass) {
    char *sql = malloc(1024);
    if (!sql) return NULL;

    snprintf(sql, 1024,
        "INSERT INTO `%s` (username, password) VALUES ('%s', '%s');",
        TABLE_NAME, name, base64_encode((unsigned char *)pass, strlen(pass)));

    return sql;
}