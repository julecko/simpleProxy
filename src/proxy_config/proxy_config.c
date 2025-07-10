#include "./core/common.h"
#include "./core/logger.h"
#include "./commands.h"
#include "./db/db.h"
#include "./core/util.h"
#include "./config.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>


int main(int argc, char *argv[]) {
    if (check_print_version(argc, argv)) {
        return EXIT_SUCCESS;
    }
    
    logger_init(stdout, stderr, LOG_DEBUG, LOG_FLAG_NO_LEVEL | LOG_FLAG_NO_TIMESTAMP);
    const char *program_name = get_program_name(argv[0]);

    if (argc < 2) {
        print_command_help(program_name);
        return EXIT_FAILURE;
    }
    const char *cmd = argv[1];

    if (strcmp(cmd, "setup") == 0) {
        int result = COMMANDS[0].func(argc, argv, (DB*)NULL);
        return result;
    }

    Config config = load_config(CONF_PATH);
    if (!check_config(&config)){
        fprintf(stderr, "Configuration file is incomplete or corrupted\n");
        return EXIT_FAILURE;
    }

    DB db;
    if (!db_create(&db, &config)) {
        fprintf(stderr, "Failed to connect to database.\n");
        return EXIT_FAILURE;
    }

    for (int i = 0; i < NUM_COMMANDS; ++i) {
        if (strcmp(cmd, COMMANDS[i].name) == 0) {
            int result = COMMANDS[i].func(argc, argv, &db);
            db_close(&db);
            return result == 0 ? EXIT_SUCCESS : EXIT_FAILURE;
        }
    }

    fprintf(stderr, "Unknown command: %s\n", cmd);
    print_command_help(program_name);
    db_close(&db);
    return EXIT_FAILURE;
}
