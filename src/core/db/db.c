#include "./db/db.h"
#include "./config.h"
#include "./core/logger.h"
#include <stdio.h>
#include <pthread.h>

bool db_create(DB *db, Config *config) {
    db->conn = mysql_init(NULL);
    if (db->conn == NULL) {
        log_error("mysql_init() failed");
        return false;
    }

    bool reconnect = 1;
    mysql_options(db->conn, MYSQL_OPT_RECONNECT, &reconnect);

    if (mysql_real_connect(db->conn, config->db_host, config->db_user, config->db_pass,
                           config->db_database, config->db_port, NULL, 0) == NULL) {
        log_error("mysql_real_connect() failed Error: %s", mysql_error(db->conn));
        mysql_close(db->conn);
        db->conn = NULL;
        return false;
    }

    pthread_mutex_init(&db->lock, NULL);
    return true;
}

MYSQL_RES *db_execute(DB *db, const char *query) {
    pthread_mutex_lock(&db->lock);

    if (mysql_query(db->conn, query)) {
        log_error("Query failed: %s", mysql_error(db->conn));
        pthread_mutex_unlock(&db->lock);
        return NULL;
    }

    MYSQL_RES *result = mysql_store_result(db->conn);

    if (result == NULL && mysql_field_count(db->conn) == 0) {
        pthread_mutex_unlock(&db->lock);
        return (MYSQL_RES *)1;
    }

    pthread_mutex_unlock(&db->lock);
    return result;
}

bool db_check_connection(DB *db) {
    return db->conn && mysql_ping(db->conn) == 0;
}

void db_print_query_result(MYSQL_RES *result) {
    if (!result) {
        log_warn("No results to print.");
        return;
    }

    unsigned int num_fields = mysql_num_fields(result);
    MYSQL_FIELD *fields = mysql_fetch_fields(result);
    MYSQL_ROW row;

    for (unsigned int i = 0; i < num_fields; i++) {
        printf("%s\t", fields[i].name);
    }
    printf("\n");

    while ((row = mysql_fetch_row(result))) {
        for (unsigned int i = 0; i < num_fields; i++) {
            printf("%s\t", row[i] ? row[i] : "NULL");
        }
        printf("\n");
    }

    mysql_free_result(result);
}

void db_close(DB *db) {
    if (db->conn) {
        mysql_close(db->conn);
        db->conn = NULL;
    }
    pthread_mutex_destroy(&db->lock);
}
