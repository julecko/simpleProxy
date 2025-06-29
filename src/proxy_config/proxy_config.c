#include "./common.h"
#include "./db/db.h"
#include "./db/user.h"
#include "./db/migration.h"
#include "./util.h"
#include "./auth.h"
#include <mysql/mysql.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <unistd.h>

DB db;

void cleanup(int signum) {
    ssize_t ignored;
    ignored = write(STDOUT_FILENO, "\nCleaning up and exiting...\n", 28);
    ignored = write(STDOUT_FILENO, "Closing database connection\n", 27);

    if (db.conn) {
        db_close(&db);
    }

    _exit(EXIT_SUCCESS);
}

void register_signal_handler(void (*handler)(int)) {
    struct sigaction sa;
    memset(&sa, 0, sizeof(sa));
    sa.sa_handler = handler;
    sa.sa_flags = 0;

    if (sigaction(SIGINT, &sa, NULL) == -1) {
        perror("sigaction SIGINT");
    }
    if (sigaction(SIGTERM, &sa, NULL) == -1) {
        perror("sigaction SIGTERM");
    }
}

void print_help_default(const char* program_name) {
    printf("Usage:\n");
    printf("  %s <command> [OPTIONS]\n", program_name);
    printf("\nCommands:\n");
    printf("  migrate              Create all necessary tables in the database.\n");
    printf("  drop                 Remove all tables from the database.\n");
    printf("  reset                Drop and re-create all tables (drop + migrate).\n");
    printf("  add -u <username>    Add a new user (password will be prompted securely).\n");
    printf("  remove -u <username> Remove an existing user (password will be prompted).\n");
    printf("  -h, --help           Show this help message and exit.\n");

    printf("\nExamples:\n");
    printf("  %s migrate\n", program_name);
    printf("  %s reset\n", program_name);
    printf("  %s add -u admin\n", program_name);
    printf("  %s remove -u admin\n", program_name);
}

int main(int argc, char* argv[]) {
    register_signal_handler(cleanup);
    const char *program_name = get_program_name(argv[0]);

    if (argc < 2) {
        print_help_default(program_name);
        return EXIT_FAILURE;
    }

    if (!db_create(&db)) {
        fprintf(stderr, "Failed to connect to database.\n");
        return EXIT_FAILURE;
    }

    const char *command = argv[1];
    const char *username = NULL;

    if (strcmp(command, "-h") == 0 || strcmp(command, "--help") == 0) {
        print_help_default(program_name);
        db_close(&db);
        return EXIT_SUCCESS;
    }

    if ((strcmp(command, "add") == 0) || (strcmp(command, "remove") == 0)) {
        if (argc != 4 || (strcmp(argv[2], "-u") != 0 && strcmp(argv[2], "--user") != 0)) {
            fprintf(stderr, "Error: '%s' requires -u <username>\n", command);
            print_help_default(program_name);
            db_close(&db);
            return EXIT_FAILURE;
        }
        username = argv[3];
    }

    if (strcmp(command, "migrate") == 0) {
        printf("Migrating...\n");
        char *query = db_user_migration();
        if (query) {
            MYSQL_RES *res = db_execute(&db, query);
            if (res) {
                printf("Migration successful.\n");
            } else {
                fprintf(stderr, "Migration failed or no result returned.\n");
            }
            free(query);
        }
    } 
    else if (strcmp(command, "drop") == 0) {
        printf("Dropping database...\n");
        char *query = db_drop_database(&db);
        if (query) {
            MYSQL_RES *res = db_execute(&db, query);
            if (res) {
                printf("Drop successful.\n");
            } else {
                fprintf(stderr, "Drop failed or no result returned.\n");
            }
            free(query);
        }
    }
    else if (strcmp(command, "reset") == 0) {
        printf("Resetting database (drop + migrate)...\n");
        char *drop_query = db_drop_database(&db);
        char *migrate_query = db_user_migration();
        if (drop_query && migrate_query) {
            MYSQL_RES *res_drop = db_execute(&db, drop_query);
            MYSQL_RES *res_migrate = db_execute(&db, migrate_query);
            if (res_drop && res_migrate) {
                printf("Reset successful.\n");
            } else {
                fprintf(stderr, "Reset failed or no results returned.\n");
            }
            free(drop_query);
            free(migrate_query);
        }
    }
    else if (strcmp(command, "add") == 0) {
        printf("Adding user: %s\n", username);
        char* password = get_password("Password: ");
        char* query = db_user_add(username, password);
        free(password);
        if (query) {
            MYSQL_RES *res = db_execute(&db, query);
            if (res) {
                printf("Adding successful.\n");
            } else {
                fprintf(stderr, "Adding of user failed or no result returned.\n");
            }
            free(query);
        }
    } 
    else if (strcmp(command, "remove") == 0) {
        printf("Removing user: %s\n", username);
        char* query = db_user_remove(username);
        if (query) {
            MYSQL_RES *res = db_execute(&db, query);
            if (res) {
                printf("Removing successful.\n");
            } else {
                fprintf(stderr, "Removing of user failed or no result returned.\n");
            }
            free(query);
        }
    }
    else if (strcmp(command, "test") == 0) {
        printf("Testing\n");
        char *query = db_user_get("Julko");
        if (query) {
            MYSQL_RES *res = db_execute(&db, query);
            if (res) {
                printf("Testing successful.\n");
                User* user = get_user_from_b64("dXNlcjpwYXNz");
                if (user){
                    printf("Username: %s\nPassword: %s\n", user->username, user->password);
                    printf("Decoded: %s", base64_decode("dXNlcjpwYXNz", strlen("dXNlcjpwYXNz")));
                    db_user_free(user);
                }
            } else {
                fprintf(stderr, "Testing failed.\n");
            }
            free(query);
        }
    }
    else {
        fprintf(stderr, "Unknown command: %s\n", command);
        print_help_default(program_name);
        db_close(&db);
        return EXIT_FAILURE;
    }

    db_close(&db);
    return EXIT_SUCCESS;
}
