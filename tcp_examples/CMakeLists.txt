cmake_minimum_required(VERSION 2.8.9)
project (tcp_server)
add_executable(tcp_server tcp_server.c Socket.c Socket.h)
project (tcp_client)
add_executable(tcp_client tcp_client.c Socket.c Socket.h)
project (discard_server)
add_executable(discard_server discard_server.c Socket.c Socket.h)
project (echo_server)
add_executable(echo_server echo_server.c Socket.c Socket.h)
project (chargen_server)
add_executable(chargen_server chargen_server.c Socket.c Socket.h)