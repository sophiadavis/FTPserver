#include "ftpServer.h"

int open_and_bind_socket_on_port(int port);
struct sockaddr_in use_ipv4_address(int port);
struct addrinfo set_socket_address_information(int port, struct sockaddr_in address_in);
int create_socket_with_port_and_address(int port, struct addrinfo address);
void set_reuse_option_on_port(int listening_socket);
int bind_with_error_checking(int listening_socket, struct sockaddr_in address_in);
int open_socket_for_incoming_connection(int listening_socket);
void spawn_thread(int new_socket, void *on_create_function);
void infinite_listen_on_socket(int listening_socket, int backlog);