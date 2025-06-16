#include <stdio.h>
#include <stdlib.h>
#include <regex.h>
#include <string.h>


char* parse_address(const char* request) {
    regex_t regex;
    regmatch_t matches[3];

    const char* pattern = "^(\\w+)\\s+([^ ]+)";

    if (regcomp(&regex, pattern, REG_EXTENDED) != 0) {
        printf("Failed to compile regex.\n");
        return NULL;
    }

    int reti = regexec(&regex, request, 3, matches, 0);
    regfree(&regex);

    if (reti != 0) {
        printf("No match found.\n");
        return NULL;
    }

    size_t length = matches[2].rm_eo - matches[2].rm_so;

    char* address = (char*)malloc(length + 1);
    if (!address){
        printf("Allocation for address failed\n");
        return NULL;
    }
    strncpy(address, request + matches[2].rm_so, length);
    address[length] = '\0';

    return address;
}