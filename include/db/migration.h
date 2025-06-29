#ifndef DB_MIGRATION_H
#define DB_MIGRATION_H

#include <stdlib.h>
#include <mysql/mysql.h>
#include "./db/db.h"

typedef struct {
    const char *name;
    const char *type;
    int length;
    const char *extra;
} Column;

typedef struct {
    const char *name;
    Column *columns;
    size_t column_count;
} Table;

char *create_table_sql(Table *table);
char *db_drop_database(DB *db);

#endif