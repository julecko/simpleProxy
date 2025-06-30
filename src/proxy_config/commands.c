#include "./proxy_config/commands.h"
#include "./db/user.h"
#include "./db/migration.h"
#include "./util.h"
#include "./auth.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>

int cmd_migrate(int argc, char **argv, DB *db) {
    printf("Migrating...\n");

    char *query = db_user_migration();
    if (!query) {
        fprintf(stderr, "Migration failed: could not create query.\n");
        return 1;
    }
    MYSQL_RES *res = db_execute(db, query);
    free(query);

    if (res == (MYSQL_RES *)1){
        printf("Migration successful.\n");
        return 0;
    } else {
        fprintf(stderr, "Migration failed: query execution failed.\n");
        return 1;
    }
}

int cmd_drop(int argc, char **argv, DB *db) {
    printf("Dropping database...\n");

    char *query = db_drop_database(db);
    if (!query) {
        fprintf(stderr, "Drop failed: could not create query.\n");
        return 1;
    }
    MYSQL_RES *res = db_execute(db, query);
    free(query);

    if (res == (MYSQL_RES *)1){
        printf("Drop successful.\n");
        return 0;
    } else {
        fprintf(stderr, "Drop failed: query execution failed.\n");
        return 1;
    }
}

int cmd_reset(int argc, char **argv, DB *db) {
    printf("Resetting database...\n");

    char *drop = db_drop_database(db);
    char *migrate = db_user_migration();

    if (!drop || !migrate) {
        fprintf(stderr, "Failed to create queries for reset.\n");
        free(drop);
        free(migrate);
        return 1;
    }

    MYSQL_RES *res1 = db_execute(db, drop);
    MYSQL_RES *res2 = db_execute(db, migrate);

    int success = 0;
    if (res1 == (MYSQL_RES *)1 && res2 == (MYSQL_RES *)1) {
        printf("Reset successful.\n");
        success = 1;
    } else {
        fprintf(stderr, "Reset failed: query execution failed.\n");
    }

    free(drop);
    free(migrate);

    return success ? 0 : 1;
}

int cmd_add(int argc, char **argv, DB *db) {
    if (argc != 4 || (strcmp(argv[2], "-u") != 0 && strcmp(argv[2], "--user") != 0)) {
        fprintf(stderr, "Usage: %s add -u <username>\n", argv[0]);
        return 1;
    }
    const char *username = argv[3];
    printf("Adding user: %s\n", username);

    char *password = get_password("Password: ");
    if (!password) {
        fprintf(stderr, "Failed to read password.\n");
        return 1;
    }

    char *query = db_user_add(username, password);
    free(password);

    if (!query) {
        fprintf(stderr, "Failed to create add user query.\n");
        return 1;
    }

    MYSQL_RES *res = db_execute(db, query);
    free(query);

    if (res == (MYSQL_RES *)1) {
        printf("User added.\n");
        return 0;
    } else {
        fprintf(stderr, "Add failed: query execution failed.\n");
        return 1;
    }
}

int cmd_remove(int argc, char **argv, DB *db) {
    if (argc != 4 || (strcmp(argv[2], "-u") != 0 && strcmp(argv[2], "--user") != 0)) {
        fprintf(stderr, "Usage: %s remove -u <username>\n", argv[0]);
        return 1;
    }
    const char *username = argv[3];
    printf("Removing user: %s\n", username);

    char *query = db_user_remove(username);
    if (!query) {
        fprintf(stderr, "Failed to create remove user query.\n");
        return 1;
    }

    MYSQL_RES *res = db_execute(db, query);
    free(query);

    if (res == (MYSQL_RES *)1) {
        printf("User removed.\n");
        return 0;
    } else {
        fprintf(stderr, "Remove failed: query execution failed.\n");
        return 1;
    }
}

const CommandEntry COMMANDS[] = {
    {"migrate", "Create all necessary tables", cmd_migrate},
    {"drop", "Remove all tables", cmd_drop},
    {"reset", "Drop and re-create all tables", cmd_reset},
    {"add", "Add a new user", cmd_add},
    {"remove", "Remove an existing user", cmd_remove},
};

const size_t NUM_COMMANDS = sizeof(COMMANDS) / sizeof(CommandEntry);

void print_command_help(const char *program_name) {
    printf("Usage: %s <command> [OPTIONS]\n\n", program_name);
    printf("Available commands:\n");
    for (int i = 0; i < NUM_COMMANDS; i++) {
        printf("  %-10s %s\n", COMMANDS[i].name, COMMANDS[i].description);
    }
    printf("\nExamples:\n");
    printf("  %s migrate\n", program_name);
    printf("  %s add -u admin\n", program_name);
}
