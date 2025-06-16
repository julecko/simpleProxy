#include "proxy.h"
#include <stdio.h>
#include <unistd.h>

int main() {
    int server_sock = create_server_socket(8080, 5);
    if (server_sock < 0) return 1;

    printf("Proxy server running on port 8080...\n");

    while (1) {
        int client_sock = accept_connection(server_sock);
        if (client_sock < 0) continue;

        handle_client(client_sock);
    }

    close(server_sock);
    return 0;
}
