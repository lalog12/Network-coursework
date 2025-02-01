#include "linkedlist.h"




linkedList* linkedList_init(){

    linkedList *list = malloc(sizeof(linkedList));

    if (list != NULL){
        list -> head = NULL;
        list -> tail = NULL;
    }
    return list;
}

uint8_t addHandle(linkedList* linkedList, char * handleName, int socketNumber){
 
    Node* current = linkedList->head;   //check if node already exists

    while (current != NULL) {

        if (strcmp(current->handleName, handleName) == 0) {
            printf("Handle name '%s' already exists\n", handleName);
            return 3;
        }
        if (current->socketNumber == socketNumber) {
            printf("Socket number %d already exists\n", socketNumber);
            return 3;
        }
        current = current->next;
    }


    Node *new_node = malloc(sizeof(Node));

    new_node -> handleName = strdup(handleName);
    new_node -> socketNumber = socketNumber;
    new_node -> next = NULL;

    if(linkedList -> head == NULL){  // empty list
        linkedList -> head = new_node;
        linkedList -> tail = new_node;
        return 2;
    }

    linkedList -> tail -> next = new_node;
    linkedList -> tail = new_node;
    return 2;
}

void removeHandle(linkedList *linkedList, char *handleName, int socketNumber){

    if(linkedList -> head == NULL){ // Empty list
        return;
    }

    Node *current = linkedList -> head;
    Node* prev = NULL;

    while (current != NULL){
        if (strcmp(current -> handleName, handleName) == 0){

            if (prev == NULL){ // removing head
                linkedList -> head = current -> next;

                if(linkedList -> head == NULL){ // if the next node is none than empty list
                    linkedList -> tail = NULL;
                }
            }

            else if (current -> next == NULL){ // checks if we're on last node
                linkedList -> tail = prev;      // new tail
                prev -> next = NULL;
            }

            else{ // removing node from the middle of the list
                prev -> next = current -> next;
            }
            free(current ->handleName);
            free(current);
            return;
        }
        prev = current;
        current = current -> next;
    }

}

Node *lookUpsocket(linkedList *linkedList, int socketNumber){
    if(linkedList -> head == NULL){
        return NULL;
    }

    Node *curr = linkedList -> head;

    while(curr != NULL){
        if(curr ->socketNumber == socketNumber){
            return curr;
        }

        curr = curr -> next;
    }
    return NULL;

}

Node *lookUphandle(linkedList *linkedList, char *handleName){
    if(linkedList -> head == NULL){
        return NULL;
    }
 
    Node *curr = linkedList -> head;

    while(curr != NULL){
        if(strcmp(curr ->handleName, handleName) == 0){
            return curr;
        }

        curr = curr -> next;
    }
    return NULL;

}


