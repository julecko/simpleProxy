#include "./core/common.h"
#include "./commands.h"
#include "./setup.h"
#include "./db/user.h"
#include "./db/migration.h"
#include "./core/util.h"
#include <stdio.h>
#include <string.h>
#include <stddef.h>
#include <stdlib.h>
#include <unistd.h>

static int run_systemctl(const char *action, const char *service_name);

int cmd_setup(int argc, char **argv, DB *db) {
    printf("Setting up proxy configurations...\n");

    if (geteuid() != 0) {
        fprintf(stderr,
            "\nThis action requires root permissions.\n"
            "Please run this command with sudo:\n\n"
            "sudo simpleproxy-config setup\n\n");
        return EXIT_FAILURE;;
    }

    char buffer[256];
    bool success;
    Config config = create_default_config();

    config.port = ask_port(buffer, sizeof(buffer));
    if (config.port == -1) {
        return EXIT_FAILURE;
    } else {
        printf("Selected port -> %d\n", config.port);
    }

    config.db_port = ask_db_port(buffer, sizeof(buffer));
    if (config.db_port == -1) {
        return EXIT_FAILURE;
    } else {
        printf("Selected database port -> %d\n", config.db_port);
    }

    config.db_host = ask_db_host(buffer, sizeof(buffer));
    if (!config.db_host) {
        return EXIT_FAILURE;
    } else {
        printf("Selected database host -> %s\n", config.db_host);
    }

    config.db_database = ask_db_database(buffer, sizeof(buffer));
    if (!config.db_database) {
        return EXIT_FAILURE;
    } else {
        printf("Selected database name -> %s\n", config.db_database);
    }

    config.db_user = ask_db_user(buffer, sizeof(buffer));
    if (!config.db_user) {
        return EXIT_FAILURE;
    } else {
        printf("Selected database user -> %s\n", config.db_user);
    }

    config.db_pass = ask_db_pass(buffer, sizeof(buffer));
    if (!config.db_pass) {
        return EXIT_FAILURE;
    } else {
        printf("Selected database password -> %s\n", config.db_pass);
    }

    if (!update_config_file(CONF_PATH, &config)){
        printf("Configurating failed\n");
        printf("In order for proxy to work correctly configure it yourself by editing " CONF_PATH "\n");
        return EXIT_FAILURE;
    } else {
        printf("Configurations successfully set up\n");
    }

    if (prompt_and_enable_service()){
        return EXIT_FAILURE;
    } else {
        return EXIT_SUCCESS;
    }
}

int cmd_setup_service(int argc, char **argv, DB *db) {
    return start_enable_service();
}

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
    {"setup", "Sets up configurations for proxy", cmd_setup},
    {"setup-service", "Sets up only systemd service for proxy", cmd_setup_service},
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
        printf("  %-15s %s\n", COMMANDS[i].name, COMMANDS[i].description);
    }
    printf("  %-15s %s\n", "-v / --version", "Prints version of simpleproxy package");
    printf("\nExamples:\n");
    printf("  %s migrate\n", program_name);
    printf("  %s add -u admin\n", program_name);
}

static int run_systemctl(const char *action, const char *service_name) {
    if (!action || !service_name) {
        fprintf(stderr, "Invalid systemctl action or service name\n");
        return -1;
    }

    char command[256];
    snprintf(command, sizeof(command), "systemctl %s %s", action, service_name);

    printf("Running: %s\n", command);

    int result = system(command);
    if (result == -1) {
        perror("Failed to execute system command");
        return -1;
    }

    if (WIFEXITED(result) && WEXITSTATUS(result) == 0) {
        printf("systemctl %s %s succeeded.\n", action, service_name);
        return 0;
    } else {
        fprintf(stderr, "systemctl %s %s failed with exit code %d\n",
                action, service_name, WEXITSTATUS(result));
        return -1;
    }
}