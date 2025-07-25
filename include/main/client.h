#ifndef MAIN_CLIENT_H
#define MAIN_CLIENT_H

#include <stddef.h>
#include <time.h>
#include <stdbool.h>
#include <stddef.h>

#define MAX_BUFFER_SIZE 4096

typedef enum {
    READING_REQUEST,
    AUTHENTICATING,
    INITIALIZE_CONNECTION,
    CONNECTING,
    FORWARDING,
    CLOSING,
    CLOSED
} ClientStateEnum;

typedef struct {
    int client_fd;
    int target_fd;

    bool client_closed;
    bool target_closed;

    size_t slot;

    time_t last_active;
    bool expired;

    char *request_buffer;
    size_t request_len;
    size_t request_capacity;

    char *response_buffer;
    size_t response_len;
    size_t response_capacity;

    ClientStateEnum state;
    char host[256];
    int port;

    int is_https;
} ClientState;

ClientState *create_client_state(int client_fd);
void free_client_state(ClientState **state_ptr, int epoll_fd);

#ifdef DEBUG_MODE
void print_client_state(const ClientState *state);
#else
static inline void print_client_state(const ClientState *state) {
    (void)state;
}
#endif

#endif