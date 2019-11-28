#ifndef CONNECT_FOUR_CLIENT_CONNECT_FOUR_LIB_H
#define CONNECT_FOUR_CLIENT_CONNECT_FOUR_LIB_H

#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "Socket.h"
#include "4clib.h"
#include <string.h>

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

typedef struct {
    uint16_t type;
    uint16_t length;
} connect_four_header_t;

typedef struct {
    char* data;
} connect_four_error_content_t;

typedef struct {
    uint32_t seq;
    uint16_t column;
    char padding[2];
} connect_four_set_column_content_t;

typedef struct {
    uint32_t seq;
} connect_four_set_column_ack_content_t;

typedef struct {
    connect_four_header_t header;
    connect_four_set_column_content_t set_column;
} connect_four_set_column_message_t;

typedef struct {
    connect_four_header_t header;
    connect_four_set_column_ack_content_t set_column_ack;
} connect_four_set_column_ack_message_t;

typedef struct {
    connect_four_header_t header;
    char* data;
} connect_four_heartbeat_ack_message_t;

typedef struct {
    connect_four_header_t header;
    char* data;
} connect_four_heartbeat_message_t;

typedef struct {
    connect_four_header_t header;
    connect_four_error_content_t error;
} connect_four_error_message_t;

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
    int socket_fd;
    int32_t other_client_port;
    uint32_t seq;
    uint16_t last_column;
    int64_t heartbeat_count;
    int64_t last_heartbeat_received;
} client_t;

void init_client(client_t* client, client_addr_t other_client_addr, client_addr_len_t other_client_addr_len, int port, int other_client_fd);

ssize_t client_send_message(client_t* client, char* buf, ssize_t len);

bool client_valid_ack(client_t* client, int seq);

int client_get_player_id(client_t* client);

void client_send_set_column(client_t* client, char buf[], uint16_t column);

void client_send_heartbeat_ack(client_t* client, char buf[], char data[], ssize_t len);

void client_send_heartbeat(client_t* client, char buf[]);

void client_send_set_column_ack(client_t* client, char buf[], uint32_t seq);

void client_send_error(client_t* client, char buf[], char* cause);

int client_get_other_player_id(client_t* client);

#endif //CONNECT_FOUR_CLIENT_CONNECT_FOUR_LIB_H
