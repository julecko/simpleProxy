#include "./main/client_registry.h"
#include "./main/client.h"
#include <stdlib.h>
#include <string.h>

static ClientState *clients[MAX_CLIENTS];

void state_registry_init() {
    memset(clients, 0, sizeof(clients));
}

size_t state_registry_add(ClientState *client) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i] == NULL) {
            clients[i] = client;
            return i;
        }
    }
    return -1;
}

void state_registry_remove(size_t slot) {
    clients[slot] = NULL;
}

void state_registry_for_each(void (*callback)(ClientState *, void *), void *arg) {
    for (int i = 0; i < MAX_CLIENTS; ++i) {
        if (clients[i]) {
            callback(clients[i], arg);
        }
    }
}
