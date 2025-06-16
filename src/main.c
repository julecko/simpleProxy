#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>

#define PORT 8080
#define BACKLOG 5

// Create and set up the TCP server socket
int create_server_socket() {
    int server_fd;
    struct sockaddr_in address;

    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == 0) {
        perror("socket failed");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(PORT);

    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("bind failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    if (listen(server_fd, BACKLOG) < 0) {
        perror("listen failed");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server listening on port %d...\n", PORT);
    return server_fd;
}

// Function to handle a single client connection
void handle_client(int client_socket) {
    char buffer[1024];
    int valread;

    printf("Client connected. Waiting for commands...\n");

    while ((valread = read(client_socket, buffer, sizeof(buffer) - 1)) > 0) {
        buffer[valread] = '\0';
        printf("Received: %s\n", buffer);

        send(client_socket, buffer, strlen(buffer), 0);
    }

    printf("Client disconnected.\n");
    close(client_socket);
}

int main() {
    int server_fd = create_server_socket();

    while (1) {
        struct sockaddr_in client_address;
        socklen_t client_addrlen = sizeof(client_address);
        int client_socket;

        if ((client_socket = accept(server_fd, (struct sockaddr *)&client_address, &client_addrlen)) < 0) {
            perror("accept failed");
            continue;
        }

        printf("Connection accepted from %s:%d\n", inet_ntoa(client_address.sin_addr), ntohs(client_address.sin_port));
        handle_client(client_socket);
    }

    close(server_fd);
    return 0;
}
