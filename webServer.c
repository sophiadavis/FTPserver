// http://blog.manula.org/2011/05/writing-simple-web-server-in-c.html
#include<netinet/in.h>    
#include<stdio.h>    
#include<stdlib.h>
#include<string.h>    
#include<sys/socket.h>    
#include<sys/stat.h>    
#include<sys/types.h>    
#include<unistd.h>  
#include <netdb.h>

int main(int argc, char *argv[]){
    
    int listening_socket, new_socket;
    
    int bufsize = 1024;
    char *buffer = malloc(bufsize);
    
    // Socket address information
        // sockaddr_in contains IPv4 information
        struct sockaddr_in address_in; 
        address_in.sin_family = AF_INET;
        address_in.sin_port = htons(8000);
        address_in.sin_addr.s_addr = htonl(INADDR_ANY);
            
        // addrinfo contains info about the socket
        struct addrinfo address;
        memset(&address, 0, sizeof(address));
        address.ai_socktype = SOCK_STREAM;
        address.ai_protocol = 0;
        address.ai_addr = (struct sockaddr *) &address_in;
        address.ai_flags = AI_PASSIVE;
    
    // For storing results from getaddrinfo
    struct addrinfo *results;
     
    int status = getaddrinfo(NULL, "8000", &address, &results);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    
// 1. Create socket
    listening_socket = socket(results->ai_family, results->ai_socktype, results->ai_protocol); // IPv4, stream, 0 = choose correct protocol for stream vs datagram 
    if (listening_socket > 0) { // success!
        printf("Socket created.\n");
    }
    
    // Allow reuse of port -- from Beej
    int yes = 1;
    if (setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }  
    
// 2. Bind socket to address
    if (bind(listening_socket, (struct sockaddr *) &address_in, sizeof(address_in)) == 0) { // socket id, *sockaddr struct w address info, length (in bytes) of address                                                         
        printf("Binding socket...\n");
    }
    else {
        printf("I'm not binding!\n");
    }
    
// 3. Listen for connections
    while (1) {
        if (listen(listening_socket, 10) < 0) { // socketfd and backlog (# requests kept waiting)
            printf("I'm not listening!\n");
            perror("server: listen");
            exit(1);
        }
        
// 4. Accept clients
        
        // Make a new socket specifically for sending/receiving data w this client
        // Info about incoming connection goes into sockaddr_storage struct 
        struct sockaddr_storage client;
        socklen_t addr_size = sizeof(client);
        if ((new_socket = accept(listening_socket, (struct sockaddr *) &client, &addr_size)) < 0) {
            perror("server: accept");
            printf("I'm not accepting!\n");
            exit(1);
        }
        
        if (new_socket > 0) {
            printf("A client has connected.\n");
        }
        
// 5. Send and receive data
        memset(buffer, 0, bufsize); // Clear buffer
        
        recv(new_socket, buffer, bufsize, 0);
        printf("%s\n", buffer);
        write(new_socket, "hello world!\n", 13);
        close(new_socket);
    }
    close(listening_socket);
    free(buffer);
    return 0;
}
