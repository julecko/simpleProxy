#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>

int main() {
    MYSQL *conn = mysql_init(NULL);

    if (conn == NULL) {
        fprintf(stderr, "mysql_init() failed\n");
        return EXIT_FAILURE;
    }

    if (mysql_real_connect(conn, "127.0.0.1", "mysql", "mysql",
                           "simpleProxy", 3306, NULL, 0) == NULL) {
        fprintf(stderr, "mysql_real_connect() failed:\nError: %s\n", mysql_error(conn));
        mysql_close(conn);
        return EXIT_FAILURE;
    }

    printf("Connected to MySQL!\n");

    if (mysql_query(conn, "SELECT VERSION()")) {
        fprintf(stderr, "Query failed: %s\n", mysql_error(conn));
        mysql_close(conn);
        return EXIT_FAILURE;
    }

    MYSQL_RES *res = mysql_store_result(conn);
    if (res) {
        MYSQL_ROW row = mysql_fetch_row(res);
        printf("MySQL version: %s\n", row[0]);
        mysql_free_result(res);
    }

    mysql_close(conn);
    return EXIT_SUCCESS;
}
