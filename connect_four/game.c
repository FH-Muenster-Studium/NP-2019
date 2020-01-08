//
// Created by Fabian Terhorst on 06.01.20.
//

#include "game.h"

void serialize_int16ToChar(char a[], uint16_t n) {
    memcpy(a, &n, 2);
}

void serialize_int32ToChar(char a[], int32_t n) {
    memcpy(a, &n, 4);
}

void serialize_uint32ToChar(char a[], uint32_t n) {
    memcpy(a, &n, 4);
}

void serialize_int64ToChar(char a[], int64_t n) {
    memcpy(a, &n, 8);
}

void serialize_uint64ToChar(char a[], uint64_t n) {
    memcpy(a, &n, 8);
}

int64_t serialize_charTo64bitNum(char a[]) {
    int64_t n = 0;
    memcpy(&n, a, 8);
    return n;
}

uint32_t serialize_charToU32bitNum(char a[]) {
    uint32_t n = 0;
    memcpy(&n, a, 4);
    return n;
}

uint16_t serialize_charToU16bitNum(char a[]) {
    uint32_t n = 0;
    memcpy(&n, a, 4);
    return n;
}

void serialize_server_read_register(char buf[], msg_reg* message, char** username, char** password) {
    message->type = serialize_charToU16bitNum(buf);
    message->length = serialize_charToU16bitNum(buf + sizeof(uint16_t));
    message->net_addr = serialize_charToU32bitNum(buf + sizeof(uint16_t) + sizeof(uint16_t));
    message->net_port = serialize_charToU16bitNum(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t));
    message->name_len =
            ntohs(serialize_charToU16bitNum(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t)));
    message->password_len = ntohs(serialize_charToU16bitNum(
            buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t)));
    char name_buffer[message->name_len];
    memcpy(name_buffer,
           buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) +
           sizeof(uint16_t), message->name_len);
    char password_buffer[message->password_len];
    memcpy(password_buffer,
           buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t) +
           sizeof(uint16_t) + message->name_len, message->password_len);
    *username = malloc(message->name_len);
    *password = malloc(message->password_len);
    memcpy(*username, name_buffer, message->name_len);
    memcpy(*password, password_buffer, message->password_len);
}

void* serialize_server_send_peer_info(msg_peer_info* msg_peer_info, ssize_t size) {
    void* buf = malloc(size);
    ssize_t name_length = size - sizeof(struct msg_peer_info_t);
    serialize_int16ToChar(buf, msg_peer_info->type);
    serialize_int16ToChar(buf + sizeof(uint16_t), msg_peer_info->length);
    serialize_uint32ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t), msg_peer_info->net_addr);
    serialize_uint32ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t), msg_peer_info->net_port);
    serialize_uint32ToChar(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t), msg_peer_info->start_flag);
    memcpy(buf + sizeof(uint16_t) + sizeof(uint16_t) + sizeof(uint32_t) + sizeof(uint16_t) + sizeof(uint16_t), msg_peer_info->data, name_length);
    return buf;
}