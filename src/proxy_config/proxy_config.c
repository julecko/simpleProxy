#include "./proxy_config/commands.h"
#include "./db/db.h"
#include "./util.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int main(int argc, char *argv[]) {
    const char *program_name = get_program_name(argv[0]);

    if (argc < 2) {
        print_command_help(program_name);
        return EXIT_FAILURE;
    }

    DB db;
    if (!db_create(&db)) {
        fprintf(stderr, "Failed to connect to database.\n");
        return EXIT_FAILURE;
    }

    const char *cmd = argv[1];
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
