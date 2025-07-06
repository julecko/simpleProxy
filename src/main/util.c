#include <fcntl.h>
#include <stdio.h>
#include <stddef.h>
#include <string.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/socket.h>

void set_nonblocking(int fd) {
    int flags = fcntl(fd, F_GETFL, 0);
    fcntl(fd, F_SETFL, flags | O_NONBLOCK);
}

int parse_host_port(const char *request, char *host, int *port) {
    const char *host_header = strstr(request, "\nHost:");
    if (!host_header)
        host_header = strstr(request, "\nhost:");
    if (!host_header) {
        fprintf(stderr, "No Host header found\n");
        return -1;
    }

    host_header += 6;
    while (*host_header == ' ' || *host_header == '\t') host_header++;

    const char *end = strchr(host_header, '\r');
    if (!end) end = strchr(host_header, '\n');
    if (!end) end = host_header + strlen(host_header);

    size_t len = end - host_header;
    char host_port[256];
    if (len >= sizeof(host_port)) len = sizeof(host_port) - 1;
    strncpy(host_port, host_header, len);
    host_port[len] = 0;

    char *colon = strchr(host_port, ':');
    if (colon) {
        *colon = 0;
        *port = atoi(colon + 1);
    } else {
        *port = 80;
    }

    strcpy(host, host_port);
    return 0;
}

int send_all(int fd, const char *buffer, size_t length) {
    ssize_t sent = 0;
    while (sent < (ssize_t)length) {
        ssize_t s = send(fd, buffer + sent, length - sent, 0);
        if (s < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            return -1;
        }
        sent += s;
    }
    return sent;
}

ssize_t recv_into_buffer(int fd, char *buffer, size_t *len, size_t capacity) {
    ssize_t n = recv(fd, buffer + *len, capacity - *len, 0);
    if (n > 0) {
        *len += n;
        return n;
    } else if (n == 0) {
        // Connection closed by peer
        return -2;
    } else if (errno == EAGAIN || errno == EWOULDBLOCK) {
        return 0; // Not an error
    } else {
        return -1; // Real error
    }
}

int send_from_buffer(int fd, char *buffer, size_t *len) {
    ssize_t sent = 0;
    while (sent < (ssize_t)(*len)) {
        ssize_t s = send(fd, buffer + sent, *len - sent, 0);
        if (s < 0) {
            if (errno == EAGAIN || errno == EWOULDBLOCK) break;
            return -1;
        }
        sent += s;
    }

    if (sent > 0) {
        memmove(buffer, buffer + sent, *len - sent);
        *len -= sent;
    }

    return 0;
}
