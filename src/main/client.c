#include "./client.h"
#include "./epoll_util.h"
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include <string.h>

ClientState *create_client_state(int client_fd, size_t slot) {
    ClientState *state = calloc(1, sizeof(ClientState));
    if (!state) return NULL;
    
    state->client_fd = client_fd;
    state->target_fd = -1;
    state->slot = slot;
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

void free_client_state(ClientState *state, int epoll_fd) {
    if (!state) return;
    if (state->client_fd != -1) {
        epoll_del_fd(epoll_fd, state->client_fd);
        close(state->client_fd);
    }
    if (state->target_fd != -1) {
        epoll_del_fd(epoll_fd, state->target_fd);
        close(state->target_fd);
    }
    free(state->request_buffer);
    free(state->response_buffer);
    free(state);
}
