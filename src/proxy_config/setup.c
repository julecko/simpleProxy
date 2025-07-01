#include "./util.h"
#include <stdio.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

#define STR_HELPER(x) #x
#define STR(x) STR_HELPER(x)

#define DEFAULT_PORT 3128
#define DEFAULT_DB_PORT 3306
#define DEFAULT_DB_DATABASE "simpleproxy"
#define DEFAULT_DB_HOST "127.0.0.1"

#define SERVICE_NAME "simpleproxy.service"

int ask_port(char *buffer, size_t buffer_size) {
    while (true) {
        bool success = get_input(buffer, buffer_size,
            "Select port on which proxy will run (DEFAULT: " STR(DEFAULT_PORT) ") -> ");

        if (!success) {
            fprintf(stderr, "Reading from input failed\n");
            return -1;
        }

        if (buffer[0] == '\0') {
            return DEFAULT_PORT;
        }

        if (!is_all_digits(buffer)) {
            printf("Invalid port number: must be between 1 and 65535.\n");
            continue;
        }

        int port = atoi(buffer);
        if (port < 0 && port > 65535) {
            printf("Invalid port number: must be between 1 and 65535.\n");
            continue;
        }

        return port;
    }
}


char *ask_db_host(char *buffer, size_t buffer_size) {
    while (true) {
        bool success = get_input(buffer, buffer_size,
            "Select host of your mysql database (DEFAULT: " DEFAULT_DB_HOST ") -> ");

        if (!success) {
            fprintf(stderr, "Reading from input failed\n");
            return NULL;
        }

        if (buffer[0] == '\0') {
            return strdup(DEFAULT_DB_HOST);
        }

        return strdup(buffer);
    }
}


int ask_db_port(char *buffer, size_t buffer_size) {
    while (true) {
        bool success = get_input(buffer, buffer_size,
            "Select port on which your mysql database is running (DEFAULT: " STR(DEFAULT_DB_PORT) ") -> ");

        if (!success) {
            fprintf(stderr, "Reading from input failed\n");
            return -1;
        }

        if (buffer[0] == '\0') {
            return DEFAULT_DB_PORT;
        }

        if (!is_all_digits(buffer)) {
            printf("Invalid port number: must be between 1 and 65535.\n");
            continue;
        }

        int port = atoi(buffer);
        if (port < 0 || port > 65535) {
            printf("Invalid port number: must be between 1 and 65535.\n");
            continue;
        }

        return port;
    }
}


char *ask_db_database(char *buffer, size_t buffer_size) {
    bool success = get_input(buffer, buffer_size,
        "Select name of your mysql database (DEFAULT: " DEFAULT_DB_DATABASE ") -> ");

    if (!success) {
        fprintf(stderr, "Reading from input failed\n");
        return NULL;
    }

    if (buffer[0] == '\0') {
        return strdup(DEFAULT_DB_DATABASE);
    }

    return strdup(buffer);
}


char *ask_db_user(char *buffer, size_t buffer_size) {
    while (true) {
        bool success = get_input(buffer, buffer_size,
            "Select username of user in database -> ");

        if (!success) {
            fprintf(stderr, "Reading from input failed\n");
            return NULL;
        }

        if (buffer[0] == '\0') {
            printf("No user selected\n");
            continue;
        }

        return strdup(buffer);
    }
}


char *ask_db_pass(char *buffer, size_t buffer_size) {
    while (true) {
        bool success = get_input(buffer, buffer_size,
            "Select password of user in database -> ");

        if (!success) {
            fprintf(stderr, "Reading from input failed\n");
            return NULL;
        }

        if (buffer[0] == '\0') {
            printf("No password selected\n");
            continue;
        }

        return strdup(buffer);
    }
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

int start_enable_service() {
    if (run_systemctl("enable", SERVICE_NAME) != 0) {
        fprintf(stderr, "Error enabling the service.\n");
        return EXIT_FAILURE;
    }

    if (run_systemctl("start", SERVICE_NAME) != 0) {
        fprintf(stderr, "Error starting the service.\n");
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}

int prompt_and_enable_service() {
    char input[16];

    printf("\nWould you like to enable and start the service automatically? [y/N]: ");
    fflush(stdout);

    if (!fgets(input, sizeof(input), stdin)) {
        fprintf(stderr, "Input error\n");
        return EXIT_FAILURE;
    }

    input[strcspn(input, "\n")] = '\0';

    if (!tolower(input[0]) == 'y') {
        printf("Okay. You can enable and start the service manually:\n");
        printf("  sudo systemctl enable %s\n", SERVICE_NAME);
        printf("  sudo systemctl start %s\n", SERVICE_NAME);
        return EXIT_SUCCESS;
    } else {
        return start_enable_service();
    }

    return EXIT_SUCCESS;
}