#include "./core/common.h"
#include "./core/logger.h"
#include "./core/util.h"
#include "./proxy.h"
#include "./db/db.h"
#include "./client.h"
#include "./main/util.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <errno.h>

#define MAX_CONNECTIONS 1024
#define BACKLOG 128

int server_sock = -1;
ClientState *clients[MAX_CONNECTIONS] = {0};

void cleanup(int signum) {
    ssize_t ignored;
    ignored = write(STDOUT_FILENO, "\nCleaning up and exiting...\n", 28);
    ignored = write(STDOUT_FILENO, "Closing socket\n", 15);
    
    if (server_sock != -1) {
        close(server_sock);
        server_sock = -1;
    }
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (clients[i]) {
            free_client_state(clients[i]);
            clients[i] = NULL;
        }
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

int find_free_slot() {
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!clients[i]) return i;
    }
    return -1;
}

size_t count_free_slots() {
    size_t counter = 0;
    for (int i = 0; i < MAX_CONNECTIONS; i++) {
        if (!clients[i]) counter++;
    }
    return counter;
}

void run_loop(DB *db) {
    set_nonblocking(server_sock);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock >= 0) {
            int slot = find_free_slot();
            if (slot == -1) {
                log_warn("Max connections reached, dropping client");
                close(client_sock);
            } else {
                set_nonblocking(client_sock);
                clients[slot] = create_client_state(client_sock, slot);
                if (!clients[slot]) {
                    log_warn("Failed to create client state");
                    close(client_sock);
                } else {
                    log_debug("Accepted new client, slot %d", slot);
                }
            }
        } else if (errno != EAGAIN && errno != EWOULDBLOCK && errno != EINTR) {
            log_error("accept failed: %s", strerror(errno));
        }

        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (clients[i]) {
                handle_client(db, clients[i]);
                if (clients[i]->state == CLOSED) {
                    clients[i] = NULL;
                    log_debug("Closed connection for slot %d", i);
                    log_debug("Free slots %ld", count_free_slots());
                }
            }
        }

        usleep(1000);
    }
}

int main(int argc, char *argv[]) {
    if (check_print_version(argc, argv)) {
        return EXIT_SUCCESS;
    }

    register_signal_handler(cleanup);

    logger_init(stdout, stderr, LOG_DEBUG, 0);

    Config config = load_config(CONF_PATH);
    if (!check_config(&config)) {
        log_error("Configuration file is incomplete or corrupted\n");
        return EXIT_FAILURE;
    }

    DB db;
    if (!db_create(&db, &config)) {
        log_error("Failed to connect to database.");
        return EXIT_FAILURE;
    }

    server_sock = create_server_socket(config.port, BACKLOG);
    if (server_sock < 0) {
        return EXIT_FAILURE;
    }

    log_info("Proxy server running on port %d...", config.port);
    fflush(stdout);

    run_loop(&db);

    close(server_sock);
    return EXIT_SUCCESS;
}
