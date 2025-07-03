#ifndef MAIN_HTTP_H
#define MAIN_HTTP_H

#include "./client.h"

int http_connect_to_target(ClientState *state);
int http_forward(ClientState *state);

#endif
