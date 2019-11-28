#include "connect_four_lib.h"

void init_client(client_t* client, client_addr_t other_client_addr, client_addr_len_t other_client_addr_len, int port,
                 int other_client_fd) {
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
    client->seq = 1;
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

/*
 int64_t charTo64bitNum(char a[]) {
  int64_t n = 0;
  n = (((int64_t)a[0] << 56) & 0xFF00000000000000U)
    | (((int64_t)a[1] << 48) & 0x00FF000000000000U)
    | (((int64_t)a[2] << 40) & 0x0000FF0000000000U)
    | (((int64_t)a[3] << 32) & 0x000000FF00000000U)
    | ((a[4] << 24) & 0x00000000FF000000U)
    | ((a[5] << 16) & 0x0000000000FF0000U)
    | ((a[6] <<  8) & 0x000000000000FF00U)
    | (a[7]        & 0x00000000000000FFU);
  return n;
}
 */

void int16ToChar(char a[], uint16_t n) {
    memcpy(a, &n, 2);
}

void int32ToChar(char a[], int32_t n) {
    memcpy(a, &n, 4);
}

void int64ToChar(char a[], int64_t n) {
    memcpy(a, &n, 8);
}

int64_t charTo64bitNum(char a[]) {
    int64_t n = 0;
    memcpy(&n, a, 8);
    return n;
}

uint32_t charToU32bitNum(char a[]) {
    uint32_t n = 0;
    memcpy(&n, a, 4);
    return n;
}

uint16_t charToU16bitNum(char a[]) {
    uint32_t n = 0;
    memcpy(&n, a, 4);
    return n;
}

void read_header(char buf[], connect_four_header_t *header) {
    header->type = ntohs(charToU16bitNum(buf));
    header->length = ntohs(charToU16bitNum(buf + sizeof(uint16_t)));
}

void read_set_column(char buf[], connect_four_set_column_message_t* message) {
    message->type = ntohs(charToU16bitNum(buf));
    message->length = ntohs(charToU16bitNum(buf + sizeof(uint16_t)));
    message->seq = ntohl(charToU32bitNum(buf + sizeof(uint16_t) +  + sizeof(uint16_t)));
    message->column = ntohs(charToU16bitNum(buf +  + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t)));
}

void read_set_column_ack(char buf[], connect_four_set_column_ack_message_t* message) {
    message->type = ntohs(charToU16bitNum(buf));
    message->length = ntohs(charToU16bitNum(buf + sizeof(uint16_t)));
    message->seq = ntohl(charToU32bitNum(buf + sizeof(uint16_t) +  + sizeof(uint16_t)));
}

void read_heartbeat(char buf[], ssize_t len, connect_four_heartbeat_message_t* message) {
    message->type = ntohs(charToU16bitNum(buf));
    message->length = ntohs(charToU16bitNum(buf + sizeof(uint16_t)));
    message->data = malloc(len);
    memcpy(message->data, buf + sizeof(uint16_t) + sizeof(uint16_t), len);
}

void read_heartbeat_ack(char buf[], ssize_t len, connect_four_heartbeat_ack_message_t* message) {
    message->type = ntohs(charToU16bitNum(buf));
    message->length = ntohs(charToU16bitNum(buf + sizeof(uint16_t)));
    message->data = malloc(len);
    memcpy(message->data, buf + sizeof(uint16_t) + sizeof(uint16_t), len);
}

void client_send_set_column(client_t* client, char buf[], uint16_t column) {
    connect_four_set_column_message_t message;
    message.type = htons(CONNECT_FOUR_HEADER_TYPE_SET_COLUMN);
    message.length = htons(sizeof(uint32_t) + sizeof(uint16_t));
    message.column = htons(column);
    message.seq = htonl(client->seq);
    memset(message.padding, 0, 2);
    int size = sizeof(message);

    int16ToChar(buf, message.type);
    int16ToChar(buf + sizeof(uint16_t), message.length);
    int32ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t), message.seq);
    int16ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t), message.column);
    buf[10] = 0;
    buf[11] = 0;
    //memcpy(buf, &message, size);
    client_send_message(client, buf, size);
}

void client_send_set_column_ack(client_t* client, char buf[], uint32_t seq) {
    connect_four_set_column_ack_message_t set_column_ack;
    set_column_ack.type = htons(CONNECT_FOUR_HEADER_TYPE_SET_COLUMN_ACK);
    set_column_ack.length = htons(sizeof(connect_four_set_column_ack_content_t));
    set_column_ack.seq = htonl(seq);
    int size = sizeof(set_column_ack);
    memcpy(buf, &set_column_ack, size);
    client_send_message(client, buf, size);
}

void client_send_heartbeat(client_t* client, char buf[]) {
    connect_four_heartbeat_message_t* message = malloc(sizeof(connect_four_header_t) + 64);
    message->type = htons(CONNECT_FOUR_HEADER_TYPE_HEARTBEAT);
    message->length = htons(64);
    char* buffer = malloc(64);
    message->data = malloc(64);
    snprintf(buffer, 64, "%lld", client->heartbeat_count);
    memcpy(message->data, buffer, 64);
    ssize_t size = sizeof(connect_four_header_t) + 64;
    memcpy(buf, message, size);

    //memcpy(buf, )

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
    message->type = htons(CONNECT_FOUR_HEADER_TYPE_HEARTBEAT_ACK);
    message->length = htons(len);
    message->data = data;
    ssize_t size = sizeof(connect_four_header_t) + len;
    memcpy(buf, message, size);
    client_send_message(client, buf, size);
    free(message);
}

void client_send_error(client_t* client, char buf[], char* cause) {
    ssize_t string_length = strlen(cause) + 1;
    connect_four_error_message_t message;
    message.type = htons(CONNECT_FOUR_HEADER_TYPE_ERROR);
    message.length = htons(string_length);
    memcpy(message.data, &cause, sizeof(cause));
    ssize_t size = sizeof(connect_four_header_t) + string_length;
    memcpy(buf, &message, size);
    client_send_message(client, buf, size);
}