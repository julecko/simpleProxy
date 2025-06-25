#include "./collections/hashset.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#define INITIAL_CAPACITY 8
#define LOAD_FACTOR 0.75

void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *safe_calloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    if (!ptr) {
        fprintf(stderr, "Memory allocation failed. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

char *safe_strdup(const char *s) {
    char *dup = strdup(s);
    if (!dup) {
        fprintf(stderr, "Memory allocation failed. Exiting.\n");
        exit(EXIT_FAILURE);
    }
    return dup;
}

static size_t hash(const char *key, size_t capacity) {
    size_t h = 0;
    while (*key) {
        h = h * 31 + (unsigned char)(*key++);
    }
    return h % capacity;
}

HashSet *hashset_create(){
    HashSet *set = safe_malloc(sizeof(HashSet));
    set->capacity = INITIAL_CAPACITY;
    set->size = 0;
    set->buckets = safe_calloc(set->capacity, sizeof(Node *));
    return set;
};

void hashset_free(HashSet *set){
    for (size_t i = 0;i < set->capacity; i++){
        Node *node = set->buckets[i];
        while (node){
            Node *next = node->next;
            free(node->value);
            free(node);
            node = next;
        }
    }
    free(set->buckets);
    free(set);
}

bool hashset_contains(HashSet *set, const char *value){
    size_t index = hash(value, set->capacity);
    const Node *node = set->buckets[index];
    while (node) {
        if (strcmp(node->value, value) == 0) {
            return true;
        }
        node = node->next;
    }
    return false;
}

static void hashset_rehash(HashSet *set){
    size_t old_capacity = set->capacity;
    Node **old_buckets = set->buckets;

    set->capacity *= 2;
    set->buckets = safe_calloc(set->capacity, sizeof(Node *));
    set->size = 0;

    for (size_t i = 0;i < old_capacity; i++){
        Node *node = old_buckets[i];
        while (node){
            hashset_add(set, node->value);
            Node *temp = node;
            node = node->next;

            free(temp->value);
            free(temp);
        }
    }
    free(old_buckets);
}

void hashset_add(HashSet *set, const char* value){
    if (hashset_contains(set, value)) return;

    if ((double)(set->size + 1) / set->capacity > LOAD_FACTOR){
        hashset_rehash(set);
    }

    size_t index = hash(value, set->capacity);
    Node *new_node = safe_malloc(sizeof(Node));

    new_node->value = safe_strdup(value);

    new_node->next = set->buckets[index];
    set->buckets[index] = new_node;

    set->size++;
}

void hashset_remove(HashSet *set, const char* value){
    size_t index = hash(value, set->capacity);
    Node *node = set->buckets[index];
    Node *prev = NULL;

    while (node) {
        if (strcmp(node->value, value) == 0){
            if (prev){
                prev->next = node->next;
            } else {
                set->buckets[index] = node->next;
            }
            free(node->value);
            free(node);
            set->size--;
            return;
        }
        prev = node;
        node = node->next;
    }
}

static void hashset_print(HashSet *set){
    for (size_t i = 0;i < set->capacity; i++){
        printf("Bucket %zu: ", i);
        Node *node = set->buckets[i];
        while (node){
            printf("%s -> ", node->value);
            node = node->next;
        }
        printf("NULL\n");
    }
}