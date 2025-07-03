#include "./db/migration.h"
#include "./core/util.h"
#include "./db/user.h"
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

char *db_user_get(const char* name) {
    char *query = malloc(1024);
    if (!query) {
        return NULL;
    }

    snprintf(query, 1024,
        "SELECT username, password FROM `%s` WHERE username = '%s' LIMIT 1;",
        TABLE_NAME, name);

    return query;
}

User *db_user_get_from_result(MYSQL_RES *res) {
    if (!res) return NULL;

    MYSQL_ROW row = mysql_fetch_row(res);
    if (!row) return NULL;

    char *username = row[0];
    char *password = row[1];

    if (!username || !password) return NULL;

    User *user = malloc(sizeof(User));
    if (!user) return NULL;

    user->username = strdup(username);
    user->password = strdup(password);

    if (!user->username || !user->password) {
        free(user->username);
        free(user->password);
        free(user);
        return NULL;
    }

    return user;
}

void db_user_free(User *user) {
    if (!user) return;
    free(user->username);
    free(user->password);
    free(user);
}

char *db_user_remove(const char *name){
    char *query = malloc(1024);
    if (!query) return NULL;

    snprintf(query, 1024,
        "DELETE FROM `%s` WHERE username = '%s'",
        TABLE_NAME, name);

    return query;
}