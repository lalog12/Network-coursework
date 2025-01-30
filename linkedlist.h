#ifndef LINKED_LIST_H
#define LINKED_LIST_H

/* Node: Represents one key-value pair. */
typedef struct Node {
    char *key;
    void *val;
    struct Node *next;
} Node;

#endif