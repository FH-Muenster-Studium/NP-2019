#include "gameserver_msg_size.h"

void init_min_msg_size(uint16_t* msg_min_size) {
    msg_min_size[MSG_REG] = (uint16_t) sizeof(msg_reg);
    msg_min_size[MSG_REG_ACK] = (uint16_t) sizeof(struct msg_header_t);
    msg_min_size[MSG_REG_NACK] = (uint16_t) sizeof(struct msg_header_t);
    msg_min_size[MSG_PEER_INFO] = (uint16_t) sizeof(msg_peer_info);
    msg_min_size[MSG_PEER_INFO_ACK] = (uint16_t) sizeof(struct msg_header_t);
    msg_min_size[MSG_MOVE] = (uint16_t) sizeof(msg_move);
    msg_min_size[MSG_MOVE_ACK] = (uint16_t) sizeof(msg_move_ack);
    msg_min_size[MSG_HEARTBEAT] = (uint16_t) sizeof(struct msg_header_t);
    msg_min_size[MSG_HEARTBEAT_ACK] = (uint16_t) sizeof(struct msg_header_t);
    msg_min_size[MSG_UNKNOWN_MSG_TYPE] = (uint16_t) sizeof(struct msg_header_t);
}