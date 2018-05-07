#define _GNU_SOURCE
#include <ctype.h>
#include <limits.h>
#include <math.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

typedef struct data_t {
    char *key;
    int value;
} data_t;

typedef struct list_node {
    data_t data;
    struct list_node *next;
} list_node;

typedef struct {
    list_node *first;
    list_node *last;
} list_head;

// Chained Hash Table
typedef struct {
    list_head **table;
    int size;  // size underlying array
    int count; // count of elements
} cht;

list_head *list_append(list_head *head, list_node *node);
void list_print(list_head *head);
int hash(char *key, int size);
cht *cht_init(int size);
list_node *cht_get(cht *tbl, char *key);
void cht_set(cht *tbl, char *key, int value);
void cht_get_sorted_values(cht *tbl, data_t p[]);
void cht_print(cht *tbl);

list_head *list_append(list_head *head, list_node *node) {
    if (node == NULL || head == NULL) {
        return head;
    }

    if (head->first == NULL) {
        head->last = node;
        head->first = node;
    } else {
        if (head->last == node) {
            list_node *tmp = calloc(1, sizeof(list_node));
            tmp->data = node->data;
            tmp->next = node->next;
            node = tmp;
        }
        head->last->next = node;
        head->last = node;
    }
    while (head->last->next != NULL) {
        head->last = head->last->next;
    }

    return head;
}

void list_print(list_head *head) {
    for (list_node *next = head->first; next != NULL; next = next->next) {
        printf("%s %d\n", next->data.key, next->data.value);
    }
}

/*
 * Chained Hash Table
 */
int hash(char *key, int size) {
    unsigned long long int hashval = 0;
    int i = 0;
    /* Convert our string to an integer */
    int len = strlen(key);
    while (hashval < ULLONG_MAX && i < len) {
        hashval = key[i] + 31 * hashval;
        i++;
    }
    return hashval % size;
}

cht *cht_init(int size) {
    cht *tbl = calloc(1, sizeof(cht));
    tbl->size = size;
    tbl->count = 0;

    tbl->table = calloc(size, sizeof(list_head *));

    // initialize chains in hash table
    for (int i = 0; i < size; i++) {
        list_head *l = calloc(1, sizeof(list_head));
        tbl->table[i] = l;
    }
    return tbl;
}

list_node *cht_get(cht *tbl, char *key) {
    int hash_key = hash(key, tbl->size);
    list_node *first = tbl->table[hash_key]->first;
    while (first != NULL && strcmp(key, first->data.key) != 0) {
        first = first->next;
    }
    return first;
}

void cht_set(cht *tbl, char *key, int value) {
    int hash_key = hash(key, tbl->size);
    list_node *found = cht_get(tbl, key);
    if (found == NULL) {
        list_node *node = calloc(1, sizeof(list_node));
        node->data.key = strdup(key);
        node->data.value = value;
        list_append(tbl->table[hash_key], node);
        tbl->count++;
    } else {
        found->data.value = value;
    }
}

int compare_data_t(const void *a, const void *b) {
    const data_t *da = (const data_t *)a;
    const data_t *db = (const data_t *)b;
    return (da->value < db->value) - (da->value > db->value);
}

void cht_get_sorted_values(cht *tbl, data_t p[]) {
    int idx = 0;
    for (int i = 0; i < tbl->size; i++) {
        for (list_node *next = tbl->table[i]->first; next != NULL;
             next = next->next) {
            p[idx++] = next->data;
        }
    }
    qsort(p, idx, sizeof(data_t), compare_data_t);
}
void cht_print(cht *tbl) {
    for (int i = 0; i < tbl->size; i++) {
        list_print(tbl->table[i]);
    }
}
