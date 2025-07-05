#include "./collections/hashmap.h"
#include "./db/db.h"
#include "./db/user.h"
#include "./core/util.h"
#include <string.h>
#include <ctype.h>
#include <stdlib.h>
#include <stdio.h>
#include <arpa/inet.h>

#define CACHE_TTL 3600

User *get_user_from_b64(const char* base64){
    if (!base64) return NULL;

    char* decoded = base64_decode(base64, strlen(base64));
    if (!decoded){
        fprintf(stderr, "Invalid decoded string\n");
        return NULL;
    }

    char *username = malloc(256);
    char *password = malloc(256);

    if (!username || !password) {
        free(decoded);
        free(username);
        free(password);
        return NULL;
    }

    int matched = sscanf(decoded, "%255[^:]:%255s", username, password);
    free(decoded);
    if (matched != 2){
        fprintf(stderr, "Username and password could be loaded after decoding");
        free(username);
        free(password);
        return NULL;
    }

    User *user = malloc(sizeof(User));
    if (!user) {
        free(username);
        free(password);
        return NULL;
    }

    user->username = username;
    user->password = password;

    return user;
}

char *extract_basic_auth_b64(const char *request) {
    const char *auth_header = strstr(request, "Proxy-Authorization:");
    if (auth_header) {
        auth_header += strlen("Proxy-Authorization:");
    } else {
        auth_header = strstr(request, "Authorization:");
        if (auth_header) {
            auth_header += strlen("Authorization:");
        } else {
            return NULL;
        }
    }

    while (isspace((unsigned char)*auth_header)) auth_header++;

    if (strncmp(auth_header, "Basic ", 6) != 0) {
        return NULL;
    }

    const char *b64_start = auth_header + 6;

    const char *end = strstr(b64_start, "\r\n");
    if (!end) {
        end = strchr(b64_start, '\n');
    }
    if (!end) {
        end = b64_start + strlen(b64_start);
    }

    size_t len = end - b64_start;
    char *auth_b64 = malloc(len + 1);
    if (!auth_b64) return NULL;

    memcpy(auth_b64, b64_start, len);
    auth_b64[len] = '\0';

    return auth_b64;
}

int has_valid_auth(DB *db, const char *request) {
    static HashMap *map = NULL;
    if (!map){
        map = hashmap_create();
    }

    char *auth_b64 = extract_basic_auth_b64(request);
    if (!auth_b64) return 0;
    
    time_t *ttl_ptr = hashmap_get(map, auth_b64);
    if (ttl_ptr){
        if (*ttl_ptr > time(NULL)){
            return 1;
        } else {
            hashmap_remove(map, auth_b64);
        }
    }

    User *request_user = get_user_from_b64(auth_b64);

    if (!request_user){
        free(auth_b64);
        return 0;
    }

    char *query = db_user_get(request_user->username);
    if (!query){
        free(auth_b64);
        db_user_free(request_user);
        return 0;
    }

    MYSQL_RES *res = db_execute(db, query);
    free(query);

    if (!res || res == (MYSQL_RES *)1) {
        free(auth_b64);
        db_user_free(request_user);
        return 0;
    }

    User *valid_user = db_user_get_from_result(res);
    mysql_free_result(res);

    if (!valid_user){
        free(auth_b64);
        db_user_free(request_user);
        return 0;
    }

    int valid = strcmp(request_user->password, valid_user->password) == 0;

    db_user_free(request_user);
    db_user_free(valid_user);

    if (valid){
        time_t ttl = time(NULL) + CACHE_TTL;
        hashmap_add(map, auth_b64, &ttl, sizeof(time_t));
    }
    free(auth_b64);

    return valid;
}

void send_proxy_auth_required(int client_socket) {
    const char *response =
        "HTTP/1.1 407 Proxy Authentication Required\r\n"
        "Proxy-Authenticate: Basic realm=\"Proxy\"\r\n"
        "Content-Length: 0\r\n"
        "Connection: close\r\n"
        "\r\n";
    send(client_socket, response, strlen(response), 0);
}