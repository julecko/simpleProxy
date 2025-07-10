#include "./collections/hashmap.h"
#include "./core/logger.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <pthread.h>

#define INITIAL_CAPACITY 8
#define LOAD_FACTOR 0.75


struct HashMap {
    Node **buckets;
    size_t capacity;
    size_t size;
    pthread_mutex_t lock;
};

void *safe_malloc(size_t size) {
    void *ptr = malloc(size);
    if (!ptr) {
        log_error("Memory allocation failed. Exiting.");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

void *safe_calloc(size_t count, size_t size) {
    void *ptr = calloc(count, size);
    if (!ptr) {
        log_error("Memory allocation failed. Exiting.");
        exit(EXIT_FAILURE);
    }
    return ptr;
}

char *safe_strdup(const char *s) {
    char *dup = strdup(s);
    if (!dup) {
        log_error("Memory allocation failed. Exiting.");
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

HashMap *hashmap_create(){
    HashMap *map = safe_malloc(sizeof(HashMap));
    map->capacity = INITIAL_CAPACITY;
    map->size = 0;
    map->buckets = safe_calloc(map->capacity, sizeof(Node *));
    pthread_mutex_init(&map->lock, NULL);
    return map;
};

void hashmap_free(HashMap *map){
    pthread_mutex_lock(&map->lock);
    for (size_t i = 0; i < map->capacity; i++){
        Node *node = map->buckets[i];
        while (node){
            Node *next = node->next;
            free(node->key);
            free(node->value);
            free(node);
            node = next;
        }
    }
    free(map->buckets);
    pthread_mutex_unlock(&map->lock);
    pthread_mutex_destroy(&map->lock);
    free(map);
}


bool hashmap_contains(HashMap *map, const char *key){
    size_t index = hash(key, map->capacity);
    const Node *node = map->buckets[index];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            return true;
        }
        node = node->next;
    }
    return false;
}

static void hashmap_rehash(HashMap *map){
    size_t old_capacity = map->capacity;
    Node **old_buckets = map->buckets;

    map->capacity *= 2;
    map->buckets = safe_calloc(map->capacity, sizeof(Node *));
    map->size = 0;

    for (size_t i = 0;i < old_capacity; i++){
        Node *node = old_buckets[i];
        while (node){
            hashmap_add(map, node->key, node->value, node->value_size);
            Node *temp = node;
            node = node->next;

            free(temp->key);
            free(temp->value);
            free(temp);
        }
    }
    free(old_buckets);
}

void hashmap_add(HashMap *map, const char *key, const void *value, size_t value_size){
    pthread_mutex_lock(&map->lock);

    if (hashmap_contains(map, key)) {
        pthread_mutex_unlock(&map->lock);
        return;
    }

    if ((double)(map->size + 1) / map->capacity > LOAD_FACTOR){
        hashmap_rehash(map);
    }

    size_t index = hash(key, map->capacity);
    Node *new_node = safe_malloc(sizeof(Node));

    new_node->key = safe_strdup(key);

    new_node->value = safe_malloc(value_size);
    memcpy(new_node->value, value, value_size);
    new_node->value_size = value_size;

    new_node->next = map->buckets[index];
    map->buckets[index] = new_node;

    map->size++;

    pthread_mutex_unlock(&map->lock);
}


void hashmap_remove(HashMap *map, const char* key){
    pthread_mutex_lock(&map->lock);

    size_t index = hash(key, map->capacity);
    Node *node = map->buckets[index];
    Node *prev = NULL;

    while (node) {
        if (strcmp(node->key, key) == 0){
            if (prev){
                prev->next = node->next;
            } else {
                map->buckets[index] = node->next;
            }

            free(node->key);
            free(node->value);
            free(node);

            map->size--;
            return;
        }
        prev = node;
        node = node->next;
    }

    pthread_mutex_unlock(&map->lock);
}

void *hashmap_get(HashMap *map, const char *key) {
    size_t index = hash(key, map->capacity);
    Node *node = map->buckets[index];
    while (node) {
        if (strcmp(node->key, key) == 0) {
            return node->value;
        }
        node = node->next;
    }
    return NULL;
}

void hashmap_print(HashMap *map){
    for (size_t i = 0;i < map->capacity; i++){
        printf("Bucket %zu: ", i);
        Node *node = map->buckets[i];
        while (node){
            printf("%s:%d -> ", node->key, node->value ? 1 : 0);
            node = node->next;
        }
        printf("NULL\n");
    }
}