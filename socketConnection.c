#include "ftpServer.h"
#include "response.h"
#include "socketConnection.h"

int open_and_bind_socket_on_port(int port) {
    
    struct sockaddr_in address_in = use_ipv4_address(port);
    struct addrinfo address = set_socket_address_information(port, address_in);
     
    int listening_socket = create_socket_with_port_and_address(port, address);
    
    set_reuse_port_option(listening_socket);
    
    listening_socket = bind_with_error_checking(listening_socket, address_in);
    
    return listening_socket;
}

int bind_with_error_checking(int listening_socket, struct sockaddr_in address_in) {
    
    // socket id, *sockaddr struct w address info, length (in bytes) of address
    int bind_status = bind(listening_socket, (struct sockaddr *) &address_in, sizeof(address_in));                                                          
    check_status(bind_status, "bind");
    printf("Binding socket...\n");
    
    return listening_socket;
}

struct sockaddr_in use_ipv4_address(int port) {
    struct sockaddr_in address_in; // sockaddr_in sets IPv4 information 
    address_in.sin_family = AF_INET; // IPv4
    address_in.sin_port = htons(port);
    address_in.sin_addr.s_addr = INADDR_ANY; // expects 4-byte IP address (INADDR_ANY = use my IPv4 address)
    
    return address_in;
}

struct addrinfo set_socket_address_information(int port, struct sockaddr_in address_in) {
        
    struct addrinfo address; // addrinfo contains info about the socket
    memset(&address, 0, sizeof(address));
    address.ai_socktype = SOCK_STREAM;
    address.ai_protocol = 0; // 0 = choose correct protocol for stream vs datagram
    address.ai_addr = (struct sockaddr *) &address_in;
    address.ai_flags = AI_PASSIVE; // fills in IP automatically
    
    return address;
}

int create_socket_with_port_and_address(int port, struct addrinfo address) {
    
    char port_str[7];
    sprintf(port_str, "%d", port);
    
    struct addrinfo *results;
    
    int status = getaddrinfo(NULL, port_str, &address, &results);
    
    char message[50];
    sprintf(message, "getaddrinfo error: %s\n", gai_strerror(status));
    
    check_status(status, message);
    
    int listening_socket = socket(results->ai_family, results->ai_socktype, results->ai_protocol); 
    if (listening_socket > 0) {
        printf("Socket created, listening on port %s.\n", port_str);
    }
    
    freeaddrinfo(results);
    
    return listening_socket;
}

void set_reuse_port_option(int listening_socket) {
    int yes = 1;
    int sockopt_status = setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    check_status(sockopt_status, "setsockopt");
    printf("SOCKOPT SET!!!!\n");
}

void infinite_listen_on_socket(int listening_socket, int backlog) {
    int new_socket;
    while (1) {
    
        int listen_status = listen(listening_socket, backlog);
        check_status(listen_status, "listen");
    
        new_socket = open_socket_for_incoming_connection(listening_socket);
        spawn_thread(new_socket, &process_control_connection);
    }
}

// Accept connection and spawn new thread
int open_socket_for_incoming_connection(int listening_socket) {
    
    // Make a new socket specifically for sending/receiving data w this client
    int new_socket;
    
    // Info about incoming connection goes into sockaddr_storage struct 
    struct sockaddr_storage client;
    socklen_t addr_size = sizeof(client);
    
    new_socket = accept(listening_socket, (struct sockaddr *) &client, &addr_size);
    check_status(new_socket, "accept");

    return new_socket;
}

void spawn_thread(int new_socket, void *on_create_function) {
    pid_t pID;
    pthread_t tid;
    NUM_THREADS++;
    pthread_create(&tid, NULL, on_create_function, &new_socket);
    printf("\nA client has connected (socket %d), new thread created. Total threads: %d.\n", new_socket, NUM_THREADS);
}
