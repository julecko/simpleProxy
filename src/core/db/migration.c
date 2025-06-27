#include "./db/migration.h"
#include "./db/util.h"
#include <mysql/mysql.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static inline char *column_to_sql(const Column* col) {
    char buffer[512];

    if (col->length > 0)
        snprintf(buffer, sizeof(buffer), "`%s` %s(%d) %s", col->name, col->type, col->length, col->extra ? col->extra : "");
    else
        snprintf(buffer, sizeof(buffer), "`%s` %s %s", col->name, col->type, col->extra ? col->extra : "");

    return strdup(buffer);
}

char *create_table_sql(Table *table) {
    char* sql = malloc(4096);
    if (!sql) return NULL;

    snprintf(sql, 4096, "CREATE TABLE IF NOT EXISTS `%s` (", table->name);
    size_t len = strlen(sql);

    for (size_t i = 0; i < table->column_count; i++) {
        char* col_sql = column_to_sql(&table->columns[i]);
        len += snprintf(sql + len, 4096 - len, "%s%s", col_sql, (i < table->column_count - 1) ? ", " : "");
        free(col_sql);
    }

    snprintf(sql + len, 4096 - len, ");");
    return sql;
}

char *db_drop_database(MYSQL *conn) {
    char *tables = get_all_tables(conn);
    if (!tables) {
        return NULL;
    }
    
    char *query;
    int needed = snprintf(NULL, 0, "DROP TABLE IF EXISTS %s;", tables);
    query = malloc(needed + 1);
    if (!query) {
        free(tables);
        return NULL;
    }
    sprintf(query, "DROP TABLE IF EXISTS %s;", tables);
    free(tables);
    return query;
}