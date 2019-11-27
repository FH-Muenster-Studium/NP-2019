#ifndef CONNECT_FOUR_CLIENT_CONNECT_FOUR_LIB_H
#define CONNECT_FOUR_CLIENT_CONNECT_FOUR_LIB_H

#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "Socket.h"
#include "4clib.h"

// Network protocol (messages)

#define CONNECT_FOUR_HEADER_TYPE_REGISTRATION_REQUEST 1
#define CONNECT_FOUR_HEADER_TYPE_REGISTRATION_REQUEST_ACK 2
#define CONNECT_FOUR_HEADER_TYPE_REGISTRATION_REQUEST_N_ACK 3
#define CONNECT_FOUR_HEADER_TYPE_PEER_INFO 4
#define CONNECT_FOUR_HEADER_TYPE_PEER_INFO_ACK 5
#define CONNECT_FOUR_HEADER_TYPE_SET_COLUMN 6
#define CONNECT_FOUR_HEADER_TYPE_SET_COLUMN_ACK 7
#define CONNECT_FOUR_HEADER_TYPE_HEARTBEAT 8
#define CONNECT_FOUR_HEADER_TYPE_HEARTBEAT_ACK 9
#define CONNECT_FOUR_HEADER_TYPE_ERROR 10

typedef struct connect_four_header {
    uint16_t type;
    uint16_t length;
} connect_four_header_t;

typedef struct connect_four_heartbeat {
    uint32_t count;
} connect_four_heartbeat_t;

typedef struct connect_four_heartbeat_ack {
    uint32_t count;
} connect_four_heartbeat_ack_t;

typedef struct connect_four_error {
    uint32_t cause;
} connect_four_error_t;

typedef struct connect_four_set_column {
    uint32_t seq;
    uint16_t column;
    char padding[2];
} connect_four_set_column_t;

typedef struct connect_four_set_column_ack {
    uint32_t seq;
} connect_four_set_column_ack_t;

typedef struct connect_four_set_column_header {
    connect_four_header_t header;
    connect_four_set_column_t set_column;
} connect_four_set_column_header_t;

typedef struct connect_four_set_column_ack_header {
    connect_four_header_t header;
    connect_four_set_column_ack_t set_column_ack;
} connect_four_set_column_ack_header_t;

typedef struct connect_four_message {
    connect_four_header_t header;
    union {
        connect_four_heartbeat_t heartbeat;
        connect_four_heartbeat_ack_t heartbeat_ack;
        connect_four_set_column_t set_column;
        connect_four_set_column_ack_t set_column_ack;
        connect_four_error_t error;
    };
} connect_four_message_t;

// Client states

typedef struct sockaddr* client_addr_t;
typedef socklen_t client_addr_len_t;

#define CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_CLIENT_WITH_A_FIRST_TURN 1 // started game without params
#define CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_USER_INPUT 2 // started game with params
#define CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN_ACK 3 // started game with params and waiting for ack of other client
#define CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN 4
#define CONNECT_FOUR_CLIENT_STATE_GAME_END 5
#define CONNECT_FOUR_CLIENT_STATE_ERROR -1

typedef struct client {
    int16_t state;
    bool first;
    client_addr_t other_client_addr;
    client_addr_len_t other_client_addr_len;
    int other_client_fd;
    int32_t other_client_port;
    uint32_t seq;
} client_t;

void init_client(client_t* client, client_addr_t other_client_addr, client_addr_len_t other_client_addr_len, int port, int other_client_fd);

ssize_t client_send_message(client_t* client, char* buf, ssize_t len);

bool client_valid_ack(client_t* client, int seq);

int client_get_player_id(client_t* client);

#endif //CONNECT_FOUR_CLIENT_CONNECT_FOUR_LIB_H
