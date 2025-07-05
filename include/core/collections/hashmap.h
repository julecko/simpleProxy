#ifndef HASHMAP_H
#define HASHMAP_H

#include <stdlib.h>
#include <stddef.h>
#include <stdbool.h>

typedef struct Node {
    char *key;
    void *value;
    size_t value_size;
    struct Node *next;
} Node;

typedef struct HashMap HashMap;

HashMap *hashmap_create();
void hashmap_free(HashMap *map);
bool hashmap_contains(HashMap *map, const char *key);
void hashmap_add(HashMap *map, const char *key, const void *value, size_t value_size);
void hashmap_remove(HashMap *map, const char *key);
void *hashmap_get(HashMap *map, const char *key);
void hashmap_print(HashMap *map);

#endif