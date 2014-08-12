#include "ftpServer.h"

#ifndef CONNECTION
#define CONNECTION
    typedef struct connection {
        int sign_in_status;
        int data_port;
    
        int main_socket;
        int listening_data_socket;
        int accept_data_socket;
    
        int total_bytes_sent;
    } Connection;
#endif

int open_and_bind_socket_on_port(int port);
struct sockaddr_in use_ipv4_address(int port);
struct addrinfo set_socket_address_information(int port, struct sockaddr_in address_in);
int create_socket_with_port_and_address(int port, struct addrinfo address);
void set_reuse_option_on_port(int listening_socket);
int bind_with_error_checking(int listening_socket, struct sockaddr_in address_in);
int open_socket_for_incoming_connection(int listening_socket);
