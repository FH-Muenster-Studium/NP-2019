#include "game.h"

#ifndef CONNECT_FOUR_CLIENT_GAMESERVER_CLIENTLIST_H
#define CONNECT_FOUR_CLIENT_GAMESERVER_CLIENTLIST_H

void client_list_add(client_info* list, client_info* a_client);

void client_list_remove(client_info* list, client_info* a_client);

#endif //CONNECT_FOUR_CLIENT_GAMESERVER_CLIENTLIST_H
