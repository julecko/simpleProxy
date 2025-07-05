#ifndef MAIN_HTTPS_H
#define MAIN_HTTPS_H

#include "./client.h"

void https_send_established(ClientState *state);
int https_connect_to_target(ClientState *state);
int https_forward(ClientState *state);

#endif