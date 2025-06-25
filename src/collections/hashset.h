#ifndef HASHSET_H
#define HASHSET_H

#include <stdlib.h>
#include <stdbool.h>

typedef struct Node {
    char *value;
    struct Node *next;
} Node;

typedef struct {
    Node **buckets;
    size_t capacity;
    size_t size;
} HashSet;

HashSet *hashset_create();
void hashset_free(HashSet *set);
bool hashset_contains(HashSet *set, const char *value);
void hashset_add(HashSet *set, const char *value);
void hashset_remove(HashSet *set, const char *value);

#endif