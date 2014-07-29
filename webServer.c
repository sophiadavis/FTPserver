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
    socklen_t addrlen;
    
    int bufsize = 1024;
    char *buffer = malloc(bufsize);
    
    struct sockaddr_in address_in; 
    address_in.sin_family = AF_INET;
    address_in.sin_port = htons(15000);
    address_in.sin_addr.s_addr = INADDR_ANY;
    
    struct addrinfo address;
    memset(&address, 0, sizeof(address));
    address.ai_family = AF_INET;
    address.ai_socktype = SOCK_STREAM;
    address.ai_protocol = 0;
    address.ai_addr = (struct sockaddr *) &address_in;
    
    struct addrinfo *results;
    
    // Socket address information 
    int status = getaddrinfo(INADDR_ANY, "15000", &address, &results);
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
    
// 2. Bind socket to address (host and port)
    if (bind(listening_socket, results->ai_addr, results->ai_addrlen) == 0) { // socket id, *sockaddr struct w address info, length (in bytes) of address                                                         
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
        // Now we have a new socket specifically for sending/receiving data w this client
        // Info about incoming connection goes into &address
        addrlen = sizeof((struct sockaddr *) &address_in);
        if ((new_socket = accept(listening_socket, (struct sockaddr *) &results->ai_addr, &addrlen)) < 0) {
            perror("server: accept");
            printf("I'm not accepting!\n");
            exit(1);
        }
        
        if (new_socket > 0) {
            printf("The client is connected.\n");
        }
        
// 5. Send and receive data
        // Clear buffer
        memset(buffer, 0, bufsize);
        
        recv(new_socket, buffer, bufsize, 0);
        printf("%s\n", buffer);
        write(new_socket, "hello world!\n", 12);
        close(new_socket);
    }
    close(listening_socket);
    free(buffer);
    return 0;
}
