#ifndef MAIN_CLIENT_REGISTRY_H
#define MAIN_CLIENT_REGISTRY_H

#include "./client.h"
#include <stddef.h>

#define MAX_CLIENTS 1024

void state_registry_init();

size_t state_registry_add(ClientState *client);
void state_registry_remove(size_t slot);
void state_registry_for_each(void (*callback)(ClientState *, void *), void *arg);

#endif
