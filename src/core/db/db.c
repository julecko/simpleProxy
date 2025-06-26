#include <mysql/mysql.h>
#include <stdio.h>

MYSQL *db_create(){
    MYSQL *conn = mysql_init(NULL);

    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return NULL;
    }

    if (mysql_real_connect(conn, "127.0.0.1", "mysql", "mysql",
                           "simpleProxy", 3306, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed:\nError: %s\n", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    return conn;
}

MYSQL_RES *db_execute(MYSQL *conn, const char* query){
    if (mysql_query(conn, query)) {
        fprintf(stderr, "Query failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return NULL;
    }

    return mysql_store_result(conn);
}

void db_close(MYSQL *conn) {
    if (conn) {
        mysql_close(conn);
    }
}