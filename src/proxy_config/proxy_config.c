#include "./db/db.h"
#include "./db/user.h"
#include <stdio.h>
#include <stdlib.h>
#include <mysql/mysql.h>

int main() {
    MYSQL *conn = db_create();
    if (!conn) return EXIT_FAILURE;

    char* user_migration = db_user_migration();
    MYSQL_RES *res = db_execute(conn, user_migration);
    free(user_migration);

    if (res) {
        MYSQL_ROW row = mysql_fetch_row(res);
        mysql_free_result(res);
    }

    db_execute(conn, db_user_add("adolf", "hello"));

    db_close(conn);
    return EXIT_SUCCESS;
}
