#ifndef LINKED_LIST_H
#define LINKED_LIST_H

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>

/* Node: Represents one key-value pair. */
typedef struct Node {
    char *handleName;
    int socketNumber;
    struct Node *next;
} Node;

typedef struct linkedList {
    Node *head;
    Node *tail;
} linkedList;


linkedList* linkedList_init(void);
Node *lookUphandle(linkedList *linkedList, char *handleName);
Node *lookUpsocket(linkedList *linkedList, int socketNumber);
void removeHandle(linkedList *linkedList, char *handleName, int socketNumber);
uint8_t addHandle(linkedList* linkedList ,char * handleName, int socketNumber);

#endif