#include "./client.h"
#include "./epoll_util.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

ClientState *create_client_state(int client_fd) {
    ClientState *state = calloc(1, sizeof(ClientState));
    if (!state) return NULL;
    
    state->client_fd = client_fd;
    state->target_fd = -1;

    state->last_active = time(NULL);
    state->expired = false;

    state->client_closed = false;
    state->target_closed = false;

    state->request_buffer = malloc(MAX_BUFFER_SIZE);
    state->request_capacity = MAX_BUFFER_SIZE;
    
    state->response_buffer = malloc(MAX_BUFFER_SIZE);
    state->response_capacity = MAX_BUFFER_SIZE;
    state->state = READING_REQUEST;
    state->port = 80;
    
    memset(state->host, 0, sizeof(state->host));

    if (!state->request_buffer || !state->response_buffer) {
        free(state->request_buffer);
        free(state->response_buffer);
        free(state);
        return NULL;
    }
    
    return state;
}

void free_client_state(ClientState **state_ptr, int epoll_fd) {
    if (!state_ptr || !*state_ptr) return;

    ClientState *state = *state_ptr;

    if (state->client_fd != -1) {
        close(state->client_fd);
        state->client_fd = -1;
    }

    if (state->target_fd != -1) {
        epoll_del_fd(epoll_fd, state->target_fd);
        close(state->target_fd);
        state->target_fd = -1;
    }

    free(state->request_buffer);
    free(state->response_buffer);
    free(state);

    *state_ptr = NULL;
}

#ifdef DEBUG_MODE

static const char *state_to_str(ClientStateEnum state) {
    switch (state) {
        case READING_REQUEST: return "READING_REQUEST";
        case AUTHENTICATING: return "AUTHENTICATING";
        case INITIALIZE_CONNECTION: return "INITIALIZE_CONNECTION";
        case CONNECTING: return "CONNECTING";
        case FORWARDING: return "FORWARDING";
        case CLOSING: return "CLOSING";
        case CLOSED: return "CLOSED";
        default: return "UNKNOWN";
    }
}

void print_client_state(const ClientState *state) {
    if (!state) {
        printf("ClientState: NULL\n");
        return;
    }

    printf("=== ClientState ===\n");
    printf("client_fd       : %d\n", state->client_fd);
    printf("target_fd       : %d\n", state->target_fd);
    printf("client_closed   : %s\n", state->client_closed ? "true" : "false");
    printf("target_closed   : %s\n", state->target_closed ? "true" : "false");
    printf("expired         : %s\n", state->expired ? "true" : "false");
    printf("slot            : %zu\n", state->slot);
    printf("last_active     : %ld\n", state->last_active);
    printf("state           : %s\n", state_to_str(state->state));
    printf("host            : %s\n", state->host);
    printf("port            : %d\n", state->port);
    printf("is_https        : %s\n", state->is_https ? "true" : "false");
    printf("request_len     : %zu\n", state->request_len);
    printf("response_len    : %zu\n", state->response_len);
    printf("===================\n");
}
#endif