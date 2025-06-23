#include <arpa/inet.h>
#include <stdlib.h>
#include <stdio.h>
#include <netdb.h>
#include <unistd.h>
#include <string.h>
#include <sys/epoll.h>
#include <errno.h>

void handle_https_tunnel(int client_socket, char *target_host, int target_port) {
    printf("Establishing HTTPS tunnel to %s:%d\n", target_host, target_port);

    int target_socket = socket(AF_INET, SOCK_STREAM, 0);
    if (target_socket < 0) {
        perror("socket");
        return;
    }

    struct hostent *he = gethostbyname(target_host);
    if (!he) {
        fprintf(stderr, "Could not resolve host: %s\n", target_host);
        close(target_socket);
        return;
    }

    struct sockaddr_in server_addr = {0};
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(target_port);
    memcpy(&server_addr.sin_addr, he->h_addr_list[0], he->h_length);

    if (connect(target_socket, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        perror("connect to target failed");
        close(target_socket);
        return;
    }

    const char *response = "HTTP/1.1 200 Connection Established\r\n\r\n";
    send(client_socket, response, strlen(response), 0);

    int epfd = epoll_create1(0);
    if (epfd == -1) {
        perror("epoll_create1");
        close(target_socket);
        return;
    }

    struct epoll_event ev_client = {0}, ev_target = {0};
    ev_client.events = EPOLLIN;
    ev_client.data.fd = client_socket;

    ev_target.events = EPOLLIN;
    ev_target.data.fd = target_socket;

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, client_socket, &ev_client) == -1) {
        perror("epoll_ctl client");
        close(target_socket);
        close(epfd);
        return;
    }

    if (epoll_ctl(epfd, EPOLL_CTL_ADD, target_socket, &ev_target) == -1) {
        perror("epoll_ctl target");
        close(target_socket);
        close(epfd);
        return;
    }

    struct epoll_event events[2];
    char buffer[4096];

    while (1) {
        int nfds = epoll_wait(epfd, events, 2, 5000); // 5s timeout
        if (nfds == -1) {
            perror("epoll_wait");
            break;
        } else if (nfds == 0) {
            printf("Timeout, closing tunnel\n");
            break;
        }

        for (int i = 0; i < nfds; i++) {
            int fd = events[i].data.fd;
            int other_fd = (fd == client_socket) ? target_socket : client_socket;

            ssize_t n = recv(fd, buffer, sizeof(buffer), 0);
            if (n <= 0) {
                goto cleanup;
            }

            send(other_fd, buffer, n, 0);
        }
    }
    cleanup:
        printf("Request closed\n");
        close(target_socket);
        close(epfd);
}
