#ifndef DB_H
#define DB_H

#include <mysql/mysql.h>

MYSQL *db_create();
MYSQL_RES *db_execute(MYSQL *conn, const char* query);
void db_close(MYSQL *conn);


#endif