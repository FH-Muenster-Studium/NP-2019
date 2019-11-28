#include "connect_four_lib.h"

void init_client(client_t* client, client_addr_t other_client_addr, client_addr_len_t other_client_addr_len, int port, int other_client_fd) {
    if (other_client_addr == NULL) {
        client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_CLIENT_WITH_A_FIRST_TURN;
        client->first = false;
        client->other_client_addr = NULL;
        client->other_client_addr_len = 0;
        //client->socket_fd = 0;
        //client->seq = 0;
    } else {
        client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_USER_INPUT;
        client->first = true;
        client->other_client_addr = other_client_addr;
        client->other_client_addr_len = other_client_addr_len;
        //client->seq = 1;
    }
    client->socket_fd = other_client_fd;
    client->heartbeat_count = 0;
    client->last_heartbeat_received = 0;
    client->seq = 0;
    client->other_client_port = port;
}

ssize_t client_send_message(client_t* client, char* buf, ssize_t len) {
    return Send(client->socket_fd, buf, len, 0);
}

bool client_valid_ack(client_t* client, int seq) {
    if (client->seq/* + 1*/ == seq) return true;
    return false;
}

int client_get_player_id(client_t* client) {
    return client->first ? PLAYER_1 : PLAYER_2;
}

int client_get_other_player_id(client_t* client) {
    return client->first ? PLAYER_2 : PLAYER_1;
}

void client_send_set_column(client_t* client, char buf[], uint16_t column) {
    connect_four_set_column_message_t message;
    message.header.type = htons(CONNECT_FOUR_HEADER_TYPE_SET_COLUMN);
    message.header.length = htons(sizeof(connect_four_set_column_content_t));
    message.set_column.column = column;
    message.set_column.seq = client->seq;
    int size = sizeof(message);
    memcpy(buf, &message, size);
    client_send_message(client, buf, size);
}

void client_send_set_column_ack(client_t* client, char buf[], uint32_t seq) {
    connect_four_set_column_ack_message_t set_column_ack;
    set_column_ack.header.type = htons(CONNECT_FOUR_HEADER_TYPE_SET_COLUMN_ACK);
    set_column_ack.header.length = htons(sizeof(connect_four_set_column_ack_content_t));
    set_column_ack.set_column_ack.seq = seq;
    int size = sizeof(set_column_ack);
    memcpy(buf, &set_column_ack, size);
    client_send_message(client, buf, size);
}

void client_send_heartbeat(client_t* client, char buf[]) {
    connect_four_heartbeat_message_t* message = malloc(sizeof(connect_four_header_t) + 64);
    message->header.type = htons(CONNECT_FOUR_HEADER_TYPE_HEARTBEAT);
    message->header.length = htons(64);
    char* buffer = malloc(64);
    message->data = malloc(64);
    snprintf(buffer, 64, "%lld", client->heartbeat_count);
    memcpy(message->data, buffer, 64);
    ssize_t size = sizeof(connect_four_header_t) + 64;
    memcpy(buf, message, size);

    /*(*buf) = htons(CONNECT_FOUR_HEADER_TYPE_HEARTBEAT);
    (*(buf + (sizeof(uint16_t)))) = htons(64);
    memcpy((buf + sizeof(uint16_t) + sizeof(uint16_t)), message->data, 64);*/

    client_send_message(client, buf, size);
    free(message->data);
    free(message);
    free(buffer);
}

void client_send_heartbeat_ack(client_t* client, char buf[], char data[], ssize_t len) {
    connect_four_heartbeat_ack_message_t* message = malloc(sizeof(connect_four_header_t) + len);
    message->header.type = htons(CONNECT_FOUR_HEADER_TYPE_HEARTBEAT_ACK);
    message->header.length = htons(len);
    message->data = data;
    ssize_t size = sizeof(connect_four_header_t) + len;
    memcpy(buf, message, size);
    client_send_message(client, buf, size);
    free(message);
}

void client_send_error(client_t* client, char buf[], char* cause) {
    ssize_t string_length = strlen(cause) + 1;
    connect_four_error_message_t message;
    message.header.type = htons(CONNECT_FOUR_HEADER_TYPE_ERROR);
    message.header.length = htons(string_length);
    memcpy(message.error.data, &cause, sizeof(cause));
    ssize_t size = sizeof(connect_four_header_t) + string_length;
    memcpy(buf, &message, size);
    client_send_message(client, buf, size);
}