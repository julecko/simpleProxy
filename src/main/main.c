#include "./core/common.h"
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

#define MAX_CONNECTIONS 20

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

void run_loop(DB *db) {
    set_nonblocking(server_sock);
    
    while (1) {
        struct sockaddr_in client_addr;
        socklen_t addr_len = sizeof(client_addr);
        int client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock >= 0) {
            int slot = find_free_slot();
            if (slot == -1) {
                printf("Max connections reached, dropping client\n");
                close(client_sock);
            } else {
                set_nonblocking(client_sock);
                clients[slot] = create_client_state(client_sock);
                if (!clients[slot]) {
                    printf("Failed to create client state\n");
                    close(client_sock);
                } else {
                    printf("Accepted new client, slot %d\n", slot);
                }
            }
        } else if (errno != EAGAIN && errno != EWOULDBLOCK) {
            perror("accept");
        }

        for (int i = 0; i < MAX_CONNECTIONS; i++) {
            if (clients[i]) {
                handle_client(db, clients[i]);
                if (clients[i]->state == CLOSING) {
                    free_client_state(clients[i]);
                    clients[i] = NULL;
                }
            }
        }

        usleep(1000);
    }
}

int main() {
    register_signal_handler(cleanup);

    Config config = load_config(CONF_PATH);
    if (!check_config(&config)) {
        fprintf(stderr, "Configuration file is incomplete or corrupted\n");
        return EXIT_FAILURE;
    }

    DB db;
    if (!db_create(&db, &config)) {
        fprintf(stderr, "Failed to connect to database.\n");
        return EXIT_FAILURE;
    }

    server_sock = create_server_socket(config.port, 5);
    if (server_sock < 0) {
        return EXIT_FAILURE;
    }

    printf("Proxy server running on port %d...\n", config.port);
    fflush(stdout);

    run_loop(&db);

    close(server_sock);
    return EXIT_SUCCESS;
}
