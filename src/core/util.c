#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <stddef.h>
#include <termios.h>
#include <unistd.h>
#include <stdbool.h>
#include <ctype.h>

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

char* base64_decode(const char *input, size_t len) {
    static const unsigned char base64_dtable[256] = {
        [0 ... 255] = 0x80,
        ['A'] = 0,  ['B'] = 1,  ['C'] = 2,  ['D'] = 3,
        ['E'] = 4,  ['F'] = 5,  ['G'] = 6,  ['H'] = 7,
        ['I'] = 8,  ['J'] = 9,  ['K'] = 10, ['L'] = 11,
        ['M'] = 12, ['N'] = 13, ['O'] = 14, ['P'] = 15,
        ['Q'] = 16, ['R'] = 17, ['S'] = 18, ['T'] = 19,
        ['U'] = 20, ['V'] = 21, ['W'] = 22, ['X'] = 23,
        ['Y'] = 24, ['Z'] = 25,
        ['a'] = 26, ['b'] = 27, ['c'] = 28, ['d'] = 29,
        ['e'] = 30, ['f'] = 31, ['g'] = 32, ['h'] = 33,
        ['i'] = 34, ['j'] = 35, ['k'] = 36, ['l'] = 37,
        ['m'] = 38, ['n'] = 39, ['o'] = 40, ['p'] = 41,
        ['q'] = 42, ['r'] = 43, ['s'] = 44, ['t'] = 45,
        ['u'] = 46, ['v'] = 47, ['w'] = 48, ['x'] = 49,
        ['y'] = 50, ['z'] = 51,
        ['0'] = 52, ['1'] = 53, ['2'] = 54, ['3'] = 55,
        ['4'] = 56, ['5'] = 57, ['6'] = 58, ['7'] = 59,
        ['8'] = 60, ['9'] = 61,
        ['+'] = 62, ['/'] = 63,
        ['='] = 0
    };

    if (len % 4 != 0) return NULL;

    size_t padding = 0;
    if (len >= 2) {
        if (input[len - 1] == '=') padding++;
        if (input[len - 2] == '=') padding++;
    }

    size_t decoded_len = (len / 4) * 3 - padding;
    char *decoded = malloc(decoded_len + 1);
    if (!decoded) return NULL;

    size_t i, j;
    uint32_t accumulator = 0;
    int bits_collected = 0;

    for (i = 0, j = 0; i < len; i++) {
        unsigned char c = input[i];
        unsigned char val = base64_dtable[c];

        if (val & 0x80) return NULL;

        accumulator = (accumulator << 6) | val;
        bits_collected += 6;

        if (bits_collected >= 8) {
            bits_collected -= 8;
            decoded[j++] = (accumulator >> bits_collected) & 0xFF;
        }
    }

    decoded[decoded_len] = '\0';

    return decoded;
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

bool get_input(char *buffer, size_t buffer_len, const char *prompt) {
    printf("%s", prompt);
    fflush(stdout);

    if (!fgets(buffer, buffer_len, stdin)) {
        return false;
    } else {
        buffer[strcspn(buffer, "\n")] = '\0';
    }

    return true;
}

bool is_all_digits(const char *str) {
    if (!str || *str == '\0')
        return false;

    while (*str) {
        if (!isdigit((unsigned char)*str)) {
            return false;
        }
        str++;
    }
    return true;
}
