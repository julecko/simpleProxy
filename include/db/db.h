#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>
#include <stdbool.h>

typedef struct {
    MYSQL *conn;
} DB;

bool db_create(DB *db);
MYSQL_RES *db_execute(DB *db, const char *query);
void db_close(DB *db);
void db_print_query_result(MYSQL_RES *result);
bool db_check_connection(DB *db);

#endif