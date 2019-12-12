#include "connect_four_lib.h"

void init_client(client_t* client, client_addr_t other_client_addr, client_addr_len_t other_client_addr_len, int port,
                 int other_client_fd, int server_fd) {
    if (other_client_addr == NULL) {
        client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_CLIENT_WITH_A_FIRST_TURN;
        client->first = false;
        client->other_client_addr = NULL;
        client->other_client_addr_len = 0;
        //client->other_client_fd = 0;
        //client->seq = 0;
    } else {
        client->state = CONNECT_FOUR_CLIENT_STATE_WAITING_FOR_A_USER_INPUT;
        client->first = true;
        client->other_client_addr = other_client_addr;
        client->other_client_addr_len = other_client_addr_len;
        //client->seq = 1;
    }

    time_t msec = time(NULL) * 1000;
    client->last_heartbeat_received = msec;

    client->curr_offset = 0;
    client->server_fd = server_fd;
    client->other_client_fd = other_client_fd;
    client->heartbeat_count = 0;
    client->seq = 1;
    client->other_client_port = port;
}

ssize_t client_send_message(client_t* client, char* buf, ssize_t len) {
    return Send(client->other_client_fd, buf, len, 0);
}

ssize_t client_send_server_message(client_t* client, char* buf, ssize_t len) {
    printf("send server message:%d\n", client->server_fd);
    return Send(client->server_fd, buf, len, 0);
}

ssize_t server_send_client_message(server_client_t* server_client, char* buf, ssize_t len) {
    printf("send server to client message:%d\n", server_client->client_fd);
    return Send(server_client->client_fd, buf, len, 0);
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

void uint32ToChar(char a[], uint32_t n) {
    memcpy(a, &n, 4);
}

void int64ToChar(char a[], int64_t n) {
    memcpy(a, &n, 8);
}

void uint64ToChar(char a[], uint64_t n) {
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

void read_header(char buf[], connect_four_header_t* header) {
    header->type = ntohs(charToU16bitNum(buf));
    header->length = ntohs(charToU16bitNum(buf + sizeof(uint16_t)));
}

void read_set_column(char buf[], connect_four_set_column_message_t* message) {
    message->type = ntohs(charToU16bitNum(buf));
    message->length = ntohs(charToU16bitNum(buf + sizeof(uint16_t)));
    message->seq = ntohl(charToU32bitNum(buf + sizeof(uint16_t) + +sizeof(uint16_t)));
    message->column = ntohs(charToU16bitNum(buf + +sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t)));
}

void read_set_column_ack(char buf[], connect_four_set_column_ack_message_t* message) {
    message->type = ntohs(charToU16bitNum(buf));
    message->length = ntohs(charToU16bitNum(buf + sizeof(uint16_t)));
    message->seq = ntohl(charToU32bitNum(buf + sizeof(uint16_t) + +sizeof(uint16_t)));
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

void read_error(char buf[], connect_four_error_message_t* message, char** error) {
    message->type = ntohs(charToU16bitNum(buf));
    message->length = ntohs(charToU16bitNum(buf + sizeof(uint16_t)));
    char text_buffer[message->length];
    memcpy(text_buffer, buf + sizeof(uint16_t) + sizeof(uint16_t), message->length);
    *error = malloc(message->length);
    memcpy(*error, text_buffer, message->length);
}

void server_read_header(char buf[], connect_four_header_t* message) {
    message->type = ntohs(charToU16bitNum(buf));
    message->length = ntohs(charToU16bitNum(buf + sizeof(uint16_t)));
}

void server_read_register(char buf[], connect_four_register_request_t* message, char** username, char** password) {
    message->type = ntohs(charToU16bitNum(buf));
    message->length = ntohs(charToU16bitNum(buf + sizeof(uint16_t)));
    message->ip_address = ntohl(charToU32bitNum(buf + sizeof(uint16_t) + sizeof(uint16_t)));
    message->port = ntohs(charToU16bitNum(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t)));
    message->name_length = ntohs(
            charToU16bitNum(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t)));
    message->password_length = ntohs(charToU16bitNum(
            buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t)));
    char text_buffer[message->name_length + message->password_length];
    memcpy(text_buffer,
           buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) +
           sizeof(uint16_t), message->length - sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) +
                             sizeof(uint16_t));
    *username = malloc(message->name_length);
    *password = malloc(message->password_length);
    memcpy(*username, text_buffer, message->name_length);
    memcpy(*password, text_buffer + message->name_length, message->password_length);
}

void client_read_peer_info(char buf[], connect_four_peer_info* message, char** username) {
    message->type = ntohs(charToU16bitNum(buf));
    message->length = ntohs(charToU16bitNum(buf + sizeof(uint16_t)));
    message->ip_address = ntohl(charToU32bitNum(buf + sizeof(uint16_t) + sizeof(uint16_t)));
    message->port = ntohs(charToU16bitNum(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t)));
    message->start = ntohs(
            charToU16bitNum(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t)));
    ssize_t name_length = message->length - sizeof(connect_four_peer_info) + sizeof(connect_four_header_t);
    char text_buffer[name_length];
    memcpy(text_buffer,
           buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t),
           name_length);
    *username = malloc(name_length);
    memcpy(*username, text_buffer, name_length);
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
    client_send_message(client, buf, size);
}

void client_send_set_column_ack(client_t* client, char buf[], uint32_t seq) {
    connect_four_set_column_ack_message_t set_column_ack;
    set_column_ack.type = htons(CONNECT_FOUR_HEADER_TYPE_SET_COLUMN_ACK);
    set_column_ack.length = htons(sizeof(connect_four_set_column_ack_content_t));
    set_column_ack.seq = htonl(seq);
    int size = sizeof(set_column_ack);

    int16ToChar(buf, set_column_ack.type);
    int16ToChar(buf + sizeof(uint16_t), set_column_ack.length);
    int32ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t), set_column_ack.seq);

    client_send_message(client, buf, size);
}

void client_send_heartbeat(client_t* client, char buf[]) {
    connect_four_heartbeat_message_t message;
    message.type = htons(CONNECT_FOUR_HEADER_TYPE_HEARTBEAT);
    message.length = htons(64);
    char buffer[64];
    snprintf(buffer, 64, "%lld", client->heartbeat_count);
    ssize_t size = sizeof(connect_four_header_t) + 64;

    int16ToChar(buf, message.type);
    int16ToChar(buf + sizeof(uint16_t), message.length);
    memcpy((buf + sizeof(uint16_t) + sizeof(uint16_t)), buffer, 64);

    client_send_message(client, buf, size);
}

void client_send_heartbeat_ack(client_t* client, char buf[], char data[], ssize_t len) {
    connect_four_heartbeat_message_t message;
    message.type = htons(CONNECT_FOUR_HEADER_TYPE_HEARTBEAT_ACK);
    message.length = htons(len);
    ssize_t size = sizeof(connect_four_header_t) + len;

    int16ToChar(buf, message.type);
    int16ToChar(buf + sizeof(uint16_t), message.length);
    memcpy((buf + sizeof(uint16_t) + sizeof(uint16_t)), data, len);

    client_send_message(client, buf, size);
}

void client_send_error(client_t* client, char buf[], char* cause) {
    ssize_t string_length = strlen(cause) + 1;
    connect_four_error_message_t message;
    message.type = htons(CONNECT_FOUR_HEADER_TYPE_ERROR);
    message.length = htonll(string_length);
    ssize_t size = sizeof(connect_four_header_t) + string_length;

    int16ToChar(buf, message.type);
    int16ToChar(buf + sizeof(uint16_t), message.length);
    memcpy(buf + sizeof(uint16_t) + sizeof(uint16_t), cause, string_length);

    client_send_message(client, buf, size);
}

int client_send_register(client_t* client, char buf[], uint32_t ip, uint16_t port, char* name, char* password) {
    uint16_t name_length = strlen(name) + 1;
    uint16_t password_length = strlen(password) + 1;
    /*ssize_t ip_length = sizeof(uint32_t);
    ssize_t port_length = sizeof(uint16_t);
    uint16_t sum_length = name_length + password_length + ip_length + port_length;
    ssize_t none_full_alignment_length = sum_length % sizeof(uint32_t);
    ssize_t padding_length;
    if (none_full_alignment_length == 0) {
        padding_length = 0;
    } else {
        padding_length = sizeof(uint32_t) - none_full_alignment_length;
    }*/
    uint16_t struct_size = sizeof(connect_four_register_request_t) + name_length + password_length;
    ssize_t none_full_alignment_length = struct_size % sizeof(uint32_t);
    ssize_t padding_length;
    if (none_full_alignment_length == 0) {
        padding_length = 0;
    } else {
        padding_length = sizeof(uint32_t) - none_full_alignment_length;
    }
    ssize_t full_struct_size = struct_size + padding_length;
    connect_four_register_request_t* register_request = malloc(full_struct_size);
    register_request->type = htons(CONNECT_FOUR_HEADER_TYPE_REGISTRATION_REQUEST);

    register_request->length = htons(struct_size - sizeof(connect_four_header_t));
    register_request->ip_address = htonl(ip);
    register_request->port = htons(port);
    register_request->name_length = htons(name_length);
    register_request->password_length = htons(password_length);
    memcpy(register_request->data, name, name_length);
    memcpy(register_request->data + name_length, password, password_length);

    printf("struct name len: %d\n", name_length);
    printf("struct pwd len: %d\n", password_length);
    printf("struct padding len: %ld\n", padding_length);
    printf("struct mem len: %ld\n", sizeof(connect_four_register_request_t));
    printf("struct header len: %d\n", ntohs(register_request->length));
    printf("struct len: %ld\n", full_struct_size);

    printf("register username: %s\n", register_request->data);
    printf("register password: %s\n", register_request->data + name_length);

    //uint16_t full_length = sum_length + padding_length;

    /*int16ToChar(buf, CONNECT_FOUR_HEADER_TYPE_REGISTRATION_REQUEST);
    int16ToChar(buf + sizeof(uint16_t), htons(struct_size - sizeof(connect_four_header_t)));
    uint32ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t), ip);
    int16ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t), port);
    int16ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t), name_length);
    int16ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t), password_length);
    memcpy(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t), name, name_length);
    memcpy(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t) + name_length, password, password_length);*/

    return client_send_server_message(client, (char*) register_request, full_struct_size);
}

int server_send_peer_info(server_client_t* server_client, char buf[], uint32_t ip, uint16_t port, uint16_t start,
                          char* name) {
    uint16_t name_length = strlen(name) + 1;
    uint16_t struct_size = sizeof(connect_four_peer_info) + name_length;
    ssize_t none_full_alignment_length = struct_size % sizeof(uint32_t);
    ssize_t padding_length;
    if (none_full_alignment_length == 0) {
        padding_length = 0;
    } else {
        padding_length = sizeof(uint32_t) - none_full_alignment_length;
    }
    ssize_t full_struct_size = struct_size + padding_length;
    connect_four_peer_info* peer_info = malloc(full_struct_size);
    peer_info->type = htons(CONNECT_FOUR_HEADER_TYPE_PEER_INFO);

    peer_info->length = htons(struct_size - sizeof(connect_four_header_t));
    peer_info->ip_address = htonl(ip);
    peer_info->port = htons(port);
    peer_info->start = start;
    memcpy(peer_info->data, name, name_length);

    printf("struct name len: %d\n", name_length);
    printf("struct padding len: %ld\n", padding_length);
    printf("struct mem len: %ld\n", sizeof(connect_four_peer_info));
    printf("struct len: %ld\n", full_struct_size);

    //uint16_t full_length = sum_length + padding_length;

    /*int16ToChar(buf, CONNECT_FOUR_HEADER_TYPE_REGISTRATION_REQUEST);
    int16ToChar(buf + sizeof(uint16_t), htons(struct_size - sizeof(connect_four_header_t)));
    uint32ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t), ip);
    int16ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t), port);
    int16ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t), name_length);
    int16ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t), password_length);
    memcpy(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t), name, name_length);
    memcpy(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint16_t) + name_length, password, password_length);*/

    return server_send_client_message(server_client, (char*) peer_info, full_struct_size);
}

void init_server(server_t* server) {
    server->server_client_node = single_linked_list_init();
    server->server_client_registered_node = single_linked_list_init();
    //server->registered_client_count = 0;
    //server->state = SERVER_STATE_NO_CLIENTS;
    //server->first_client_fd = NULL;
    //server->second_client_fd = NULL;
}

void add_client(server_t* server, int fd) {
    server_client_t* client = malloc(sizeof(server_client_t));
    client->curr_offset = 0;
    client->client_fd = fd;
    client->state = SERVER_CLIENT_STATE_PENDING;
    client->name = NULL;
    client->password = NULL;
    single_linked_list_insert(server->server_client_node, fd, client);
}

void add_registered_client(server_t* server, server_client_t* client) {
    single_linked_list_insert(server->server_client_registered_node, client->client_fd, client);
}

void remove_registered_client(server_t* server, int fd) {
    void* data;
    single_linked_list_delete(server->server_client_registered_node, fd, &data);
}

void get_registered_clients(server_t* server, server_client_t* server_clients[], int count) {
    void* data[count];
    single_linked_list_get_and_delete(server->server_client_registered_node, data, count);
    for (int i = 0; i < count; i++) {
        server_clients[i] = (server_client_t*) data[i];
    }
}

void remove_client(server_t* server, int fd) {
    void* data;
    if (single_linked_list_delete(server->server_client_node, fd, &data) == true) {
        server_client_t* server_client = (server_client_t*) data;
        if (server_client->name != NULL) {
            free(server_client->name);
        }
        if (server_client->password != NULL) {
            free(server_client->password);
        }
        free(data);
    }
}

bool has_client_with_name(void* data, void* data_to_find) {
    server_client_t* server_client = (server_client_t*) data;
    if (server_client->name == NULL) return false;
    char* name_to_find = (char*) data_to_find;
    if (name_to_find == NULL) return false;
    return memcmp(server_client->name, name_to_find, strlen(data_to_find)) == 0 ? true : false;
}

bool has_client(server_t* server, char* name) {
    return single_linked_list_has_data(server->server_client_node, has_client_with_name, name);
}

server_client_t* get_client(server_t* server, int fd) {
    server_client_t* curr_server_client = single_linked_list_find(server->server_client_node, fd);
    return curr_server_client;
}

ssize_t calc_padding_of_header_len(uint16_t len) {
    ssize_t diff = len % sizeof(uint32_t);
    if (diff == 0) return 0;
    return sizeof(uint32_t) - diff;
}
