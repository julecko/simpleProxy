#include "./client.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>

ClientState *create_client_state(int client_fd) {
    ClientState *state = calloc(1, sizeof(ClientState));
    if (!state) return NULL;
    
    state->client_fd = client_fd;
    state->target_fd = -1;
    state->request_buffer = malloc(MAX_BUFFER_SIZE);
    state->request_capacity = MAX_BUFFER_SIZE;
    state->response_buffer = malloc(MAX_BUFFER_SIZE);
    state->response_capacity = MAX_BUFFER_SIZE;
    state->state = READING_REQUEST;
    state->port = 80;
    state->waiting_for_auth_retry = 0;
    
    if (!state->request_buffer || !state->response_buffer) {
        free(state->request_buffer);
        free(state->response_buffer);
        free(state);
        return NULL;
    }
    
    return state;
}

void free_client_state(ClientState *state) {
    if (!state) return;
    if (state->client_fd != -1) {
        close(state->client_fd);
    }
    if (state->target_fd != -1) {
        close(state->target_fd);
    }
    free(state->request_buffer);
    free(state->response_buffer);
    free(state);
}
