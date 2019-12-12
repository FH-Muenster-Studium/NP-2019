//
// Created by Fabian Terhorst on 11.10.17.
//

#include "single_linked_list.h"
#include <stdlib.h>
#include <stdio.h>
#include <limits.h>

struct Node* createNode(int key, struct Node* next, void* data) {
    struct Node* node = malloc(sizeof(struct Node));
    node->key = key;
    node->data = data;
    node->next = next;
    return node;
}

struct Node* single_linked_list_init() {
    return createNode(INT_MIN, NULL, NULL);
}

void single_linked_list_deinit(struct Node* singleLinkedList) {
    struct Node* oldNext;
    while (singleLinkedList != NULL) {
        oldNext = singleLinkedList->next;
        free(singleLinkedList);
        singleLinkedList = oldNext;
    }
}

bool isInvalidKey(int key) {
    return key == INT_MIN;
}

void single_linked_list_insert(struct Node* singleLinkedList, int key, void* data) {
    if (isInvalidKey(key)) {
        printf("Bitte geben Sie eine valide Zahl ein\n");
        return;
    }
    struct Node* prev = singleLinkedList;
    while (singleLinkedList != NULL) {
        if (key < singleLinkedList->key) {
            prev->next = createNode(key, prev->next, data);
            break;
        } else if (singleLinkedList->next == NULL) {
            singleLinkedList->next = createNode(key, NULL, data);
            break;
        }
        prev = singleLinkedList;
        singleLinkedList = singleLinkedList->next;
    }
}

bool single_linked_list_delete(struct Node* singleLinkedList, int key, void** data) {
    if (isInvalidKey(key)) {
        printf("Bitte geben Sie eine valide Zahl ein\n");
        return false;
    }
    struct Node* prev = singleLinkedList;
    while (singleLinkedList != NULL) {
        if (singleLinkedList->key == key) {
            *data = singleLinkedList->data;
            prev->next = singleLinkedList->next;
            free(singleLinkedList);
            return true;
        }
        prev = singleLinkedList;
        singleLinkedList = singleLinkedList->next;
    }
    return false;
}

bool single_linked_list_is_empty(struct Node* singleLinkedList) {
    return singleLinkedList->next == NULL;
}

void single_linked_list_print(struct Node* singleLinkedList) {
    //Skip list element
    singleLinkedList = singleLinkedList->next;
    int i = 1;
    while (singleLinkedList != NULL) {
        printf("%d=%d\n", i++, singleLinkedList->key);
        singleLinkedList = singleLinkedList->next;
    }
    if (i == 1) {
        printf("Die Liste ist leer\n");
    }
}

void* single_linked_list_find(struct Node* singleLinkedList, int key) {
    singleLinkedList = singleLinkedList->next;
    while (singleLinkedList != NULL) {
        if (key == singleLinkedList->key) {
            return singleLinkedList->data;
        }
        singleLinkedList = singleLinkedList->next;
    }
    return 0;
}

bool single_linked_list_has_data(struct Node* singleLinkedList, bool(* find)(void*, void*), void* data_to_find) {
    singleLinkedList = singleLinkedList->next;
    while (singleLinkedList != NULL) {
        if (find(singleLinkedList->data, data_to_find) == true) {
            return true;
        }
        singleLinkedList = singleLinkedList->next;
    }
    return false;
}

bool single_linked_list_iterate(struct Node* singleLinkedList, void(* callback)(void*, void*), void* iteration_data) {
    singleLinkedList = singleLinkedList->next;
    while (singleLinkedList != NULL) {
        callback(singleLinkedList->data, iteration_data);
        singleLinkedList = singleLinkedList->next;
    }
    return false;
}

bool single_linked_list_get_and_delete(struct Node* singleLinkedList, void* data[], int count) {
    struct Node* first = singleLinkedList;
    singleLinkedList = singleLinkedList->next;
    int curr_count = count;
    while (singleLinkedList != NULL) {
        if (curr_count == 0) {
            first->next = singleLinkedList;
            return true;
        }
        data[curr_count]
        callback(singleLinkedList->data, iteration_data);
        singleLinkedList = singleLinkedList->next;
        curr_count--;
    }
    return false;
}