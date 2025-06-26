#ifndef MIGRATION_H
#define MIGRATION_H

#include <stdlib.h>

typedef struct {
    const char* name;
    const char* type;
    int length;
    const char* extra;
} Column;

typedef struct {
    const char* name;
    Column* columns;
    size_t column_count;
} Table;

char* create_table_sql(Table *table);

#endif