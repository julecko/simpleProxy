#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <termios.h>
#include <unistd.h>

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

char* base64_encode(const unsigned char *input, size_t len) {
    static const char b64_table[] = "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    size_t output_len = 4 * ((len + 2) / 3);
    char *output = malloc(output_len + 1);
    if (!output) return NULL;

    size_t i, j;
    for (i = 0, j = 0; i < len;) {
        uint32_t octet_a = i < len ? input[i++] : 0;
        uint32_t octet_b = i < len ? input[i++] : 0;
        uint32_t octet_c = i < len ? input[i++] : 0;

        uint32_t triple = (octet_a << 16) | (octet_b << 8) | octet_c;

        output[j++] = b64_table[(triple >> 18) & 0x3F];
        output[j++] = b64_table[(triple >> 12) & 0x3F];
        output[j++] = (i > len + 1) ? '=' : b64_table[(triple >> 6) & 0x3F];
        output[j++] = (i > len)     ? '=' : b64_table[triple & 0x3F];
    }

    output[output_len] = '\0';
    return output;
}

const char* get_program_name(const char* path) {
    const char* slash = strrchr(path, '/');
    return slash ? slash + 1 : path;
}

char *get_password(const char *prompt) {
    struct termios oldt, newt;
    char *password = malloc(256);
    if (!password) return NULL;

    printf("%s", prompt);
    fflush(stdout);

    // Turn off terminal echo
    tcgetattr(STDIN_FILENO, &oldt);
    newt = oldt;
    newt.c_lflag &= ~(ECHO);
    tcsetattr(STDIN_FILENO, TCSANOW, &newt);

    if (!fgets(password, 256, stdin)) {
        free(password);
        password = NULL;
    } else {
        password[strcspn(password, "\n")] = '\0';
    }

    // Restore terminal settings
    tcsetattr(STDIN_FILENO, TCSANOW, &oldt);
    printf("\n");
    return password;
}