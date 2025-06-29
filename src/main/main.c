#include "./common.h"
#include "./proxy.h"
#include "./db/db.h"
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <string.h>
#include <pthread.h>

#define MAX_CONNECTIONS 20

typedef struct ClientArgs{
    DB *db;
    int client_sock;
} ClientArgs;

int server_sock = -1;
int active_threads = 0;
pthread_mutex_t count_mutex = PTHREAD_MUTEX_INITIALIZER;

void cleanup(int signum){
    ssize_t ignored;
    ignored = write(STDOUT_FILENO, "\nCleaning up and exiting...\n", 28);
    ignored = write(STDOUT_FILENO, "Closing socket\n", 15);
    
    if (server_sock != -1){
        close(server_sock);
        server_sock = -1;
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


void *client_thread(void *arg) {
    ClientArgs *args = (ClientArgs *)arg;

    int client_sock = args->client_sock;
    DB *db = args->db;
    free(args);

    handle_client(db, client_sock);
    close(client_sock);

    pthread_mutex_lock(&count_mutex);
    active_threads--;
    pthread_mutex_unlock(&count_mutex);

    return NULL;
}

void run_loop(DB *db, int server_sock){
    while (1) {
        pthread_mutex_lock(&count_mutex);
        int current_active = active_threads;
        pthread_mutex_unlock(&count_mutex);

        if (current_active >= MAX_CONNECTIONS) {
            sleep(1);
            continue;
        }

        ClientArgs *args = malloc(sizeof(ClientArgs));
        if (!args) {
            perror("malloc");
            continue;
        }
        args->db = db;
        args->client_sock = accept_connection(server_sock);
        if (args->client_sock < 0) {
            free(args);
            continue;
        }

        pthread_t tid;
        if (pthread_create(&tid, NULL, client_thread, args) != 0) {
            perror("pthread_create");
            close(args->client_sock);
            free(args);
            continue;
        }

        pthread_detach(tid);

        pthread_mutex_lock(&count_mutex);
        active_threads++;
        pthread_mutex_unlock(&count_mutex);
    }
}

int main() {
    register_signal_handler(cleanup);

    DB db;
    db_create(&db);

    server_sock = create_server_socket(8080, 5);

    if (server_sock < 0) return 1;

    printf("Proxy server running on port 8080...\n");

    run_loop(&db, server_sock);

    close(server_sock);
    return 0;
}
