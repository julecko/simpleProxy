#include "./core/logger.h"
#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>

char *get_all_tables(MYSQL *conn) {
    if (mysql_query(conn, 
        "SELECT GROUP_CONCAT(CONCAT('`', table_name, '`') SEPARATOR ', ') "
        "FROM information_schema.tables WHERE table_schema = DATABASE();")) {

        log_error("Failed to query tables: %s", mysql_error(conn));
        return NULL;
    }
    
    MYSQL_RES *res = mysql_store_result(conn);
    if (!res) {
        log_error("No result from table query");
        return NULL;
    }
    
    MYSQL_ROW row = mysql_fetch_row(res);
    char *tables = NULL;
    if (row && row[0]) {
        tables = strdup(row[0]);
    }
    mysql_free_result(res);
    return tables;
}