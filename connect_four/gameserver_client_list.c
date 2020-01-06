#include "gameserver_client_list.h"

void client_list_add(client_info* list, client_info* a_client) {
    for (; list->next != NULL; list = list->next) {
    }
    list->next = a_client;
}

void client_list_remove(client_info* list, client_info* a_client) {
    for (; list->next != a_client; list = list->next) {
        if (list->next == NULL) {
            printf("No client found in list\n");
            return;
        }
    }
    list->next = list->next->next;
}
