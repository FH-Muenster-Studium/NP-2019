#ifndef CONNECT_FOUR_CLIENT_CONNECT_FOUR_LIB_H
#define CONNECT_FOUR_CLIENT_CONNECT_FOUR_LIB_H

#include <stdlib.h>
#include <stdbool.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include "Socket.h"
#include "4clib.h"
#include <string.h>
#include "single_linked_list.h"

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

#define BUFFER_SIZE (1<<16)

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
    uint16_t type;
    uint16_t length;
    uint32_t seq;
    uint16_t column;
    char padding[2];
} connect_four_set_column_message_t;

typedef struct {
    uint16_t type;
    uint16_t length;
    uint32_t seq;
} connect_four_set_column_ack_message_t;

typedef struct {
    uint16_t type;
    uint16_t length;
    char data[];
} connect_four_error_message_t;

typedef struct {
    uint16_t type;
    uint16_t length;
    char* data;
} connect_four_heartbeat_ack_message_t;

typedef struct {
    uint16_t type;
    uint16_t length;
    char* data;
} connect_four_heartbeat_message_t;

typedef struct __attribute__((__packed__)) {
    uint16_t type;
    uint16_t length;
    uint32_t ip_address;
    uint16_t port;
    uint16_t name_length;
    uint16_t password_length;
    char data[]; // name - password - padding
} connect_four_register_request_t;

typedef struct __attribute__((__packed__)) {
    uint16_t type;
    uint16_t length;
} connect_four_register_ack;

typedef struct __attribute__((__packed__)) {
    uint16_t type;
    uint16_t length;
} connect_four_register_nack;

typedef struct __attribute__((__packed__)) {
    uint16_t type;
    uint16_t length;
    uint32_t ip_address;
    uint16_t port;
    uint16_t start;
    char data[]; // name - padding
} connect_four_peer_info;

typedef struct __attribute__((__packed__)) {
    uint16_t type;
    uint16_t length;
} connect_four_peer_info_ack;

// Client states

typedef struct sockaddr* client_addr_t;
typedef socklen_t client_addr_len_t;

#define CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_CLIENT_WITH_A_FIRST_TURN 1 // started game without params
#define CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_USER_INPUT 2 // started game with params
#define CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN_ACK 3 // started game with params and waiting for ack of other client
#define CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_TURN 4
#define CONNECT_FOUR_CLIENT_STATE_GAME_END 5
#define CONNECT_FOUR_CLIENT_STATE_ERROR -1

#define SERVER_CLIENT_STATE_PENDING 1
#define SERVER_CLIENT_STATE_REGISTERED 2
#define SERVER_CLIENT_STATE_PLAYING 3

typedef struct client {
    int16_t state;
    bool first;
    client_addr_t other_client_addr;
    client_addr_len_t other_client_addr_len;
    int other_client_fd;
    int32_t other_client_port;
    uint32_t seq;
    uint16_t last_column;
    int64_t heartbeat_count;
    int64_t last_heartbeat_received;
    int server_fd;

    ssize_t curr_offset;
    char message_buffer[BUFFER_SIZE];
} client_t;

typedef struct {
    int client_fd;
    char message_buffer[BUFFER_SIZE];
    ssize_t curr_offset;
    char send_buffer[BUFFER_SIZE];
    int state;
    char* name;
    char* password;
    uint32_t ip;
    uint16_t port;
} server_client_t;

typedef struct {
    int server_fd;
    struct Node* server_client_node;
    int registered_client_count;
} server_t;

void init_server(server_t* server);

void add_client(server_t* server, int fd);

void remove_client(server_t* server, int fd);

bool has_client(server_t* server, char* name);

server_client_t* get_client(server_t* server, int fd);

void init_client(client_t* client, client_addr_t other_client_addr, client_addr_len_t other_client_addr_len, int port, int other_client_fd, int server_fd);

ssize_t client_send_message(client_t* client, char* buf, ssize_t len);

ssize_t client_send_server_message(client_t* client, char* buf, ssize_t len);

bool client_valid_ack(client_t* client, int seq);

int client_get_player_id(client_t* client);

void client_send_set_column(client_t* client, char buf[], uint16_t column);

void client_send_heartbeat_ack(client_t* client, char buf[], char data[], ssize_t len);

void client_send_heartbeat(client_t* client, char buf[]);

void client_send_set_column_ack(client_t* client, char buf[], uint32_t seq);

void client_send_error(client_t* client, char buf[], char* cause);

int client_send_register(client_t* client, char buf[], uint32_t ip, uint16_t port, char* name, char* password);

int client_get_other_player_id(client_t* client);

void read_header(char buf[], connect_four_header_t *header);

void read_set_column(char buf[], connect_four_set_column_message_t* message);

void read_set_column_ack(char buf[], connect_four_set_column_ack_message_t* message);

void read_heartbeat(char buf[], ssize_t len, connect_four_heartbeat_message_t* message);

void read_heartbeat_ack(char buf[], ssize_t len, connect_four_heartbeat_ack_message_t* message);

void read_error(char buf[], connect_four_error_message_t* message, char** error);

void server_read_register(char buf[], connect_four_register_request_t* message, char** username, char** password);

void server_read_header(char buf[], connect_four_header_t* message);

int server_send_peer_info(server_client_t* server_client, char buf[], uint32_t ip, uint16_t port, uint16_t start, char* name);

void client_read_peer_info(char buf[], connect_four_peer_info* message, char** username);

ssize_t calc_padding_of_header_len(uint16_t len);

#endif //CONNECT_FOUR_CLIENT_CONNECT_FOUR_LIB_H
