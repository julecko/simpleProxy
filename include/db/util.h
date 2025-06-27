#ifndef DB_UTIL_H
#define DB_UTIL_H

#include <mysql/mysql.h>

char *get_all_tables(MYSQL *conn);

#endif