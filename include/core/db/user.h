#ifndef DB_USER_H
#define DB_USER_H

#include <mysql/mysql.h>

typedef struct User {
    char* username;
    char* password;
} User;

char *db_user_migration();
char *db_user_add(const char *name, const char *base64_pass);
char *db_user_get(const char* name);
void db_user_free(User *user);
User *db_user_get_from_result(MYSQL_RES *res);
char *db_user_remove(const char *name);

#endif