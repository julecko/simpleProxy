#include <string.h>
#include <ctype.h>
#include <arpa/inet.h>

#define AUTH_CREDENTIALS "Basic dXNlcjpwYXNz" //user:pass

int has_valid_auth(const char *request) {
    const char *auth_header = strstr(request, "Proxy-Authorization: ");
    if (!auth_header) return 0;

    auth_header += strlen("Proxy-Authorization: ");
    while (isspace(*auth_header)) auth_header++;

    if (strncmp(auth_header, AUTH_CREDENTIALS, strlen(AUTH_CREDENTIALS)) == 0) {
        return 1; //For testing purposes
    }

    return 0;
}

void send_proxy_auth_required(int client_socket) {
    const char *response =
        "HTTP/1.1 407 Proxy Authentication Required\r\n"
        "Proxy-Authenticate: Basic realm=\"Proxy\"\r\n"
        "Content-Length: 0\r\n"
        "\r\n";
    send(client_socket, response, strlen(response), 0);
}