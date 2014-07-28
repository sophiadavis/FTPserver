// http://blog.manula.org/2011/05/writing-simple-web-server-in-c.html
#include<netinet/in.h>    
#include<stdio.h>    
#include<stdlib.h>
#include<string.h>    
#include<sys/socket.h>    
#include<sys/stat.h>    
#include<sys/types.h>    
#include<unistd.h> 

int main(int argc, char *argv[]){

    // todo -- pass in port via command line
    
    int create_socket, new_socket;
    socklen_t addrlen;
    
    int bufsize = 1024;
    char *buffer = malloc(bufsize); // buffer for storing received data
    
    // 1. Create socket
    create_socket = socket(AF_INET, SOCK_STREAM, 0); // IPv4, stream, 0 = choose correct protocol for stream vs datagram 
    if (create_socket > 0) { // success!
        printf("Socket created.\n");
    }
    
    // Socket address information!
    // instead of getaddrinfo() -- http://beej.us/guide/bgnet/output/html/multipage/syscalls.html
    struct sockaddr_in address; // Contains socket address information
    address.sin_family = AF_INET; // address family
    address.sin_addr.s_addr = INADDR_ANY; // internet address (4-byte IP address (in Network Byte Order))
                                // INADDR_ANY makes socket listen at any available interfaces
    address.sin_port = htons(15000); // port number -- htons = host to network short -- don't get this! 
        // direction of reading in data (kernel expects network address, 15000 is human readable)
    printf("port: %i\n", address.sin_port);  
    
    // Allow reuse of port -- from Beej
    int yes = 1;
    if (setsockopt(create_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int)) == -1) {
        perror("setsockopt");
        exit(1);
    }  
    
    // 2. Bind socket to address (host and port)
    if (bind(create_socket, (struct sockaddr *) &address, sizeof(address)) == 0) { // socket id, *sockaddr struct w address info, length (in bytes) of address
                                                                    // -1 means error
        printf("Binding socket\n");
    }
    
    // 3. Listen for connections
    while (1) {
        if (listen(create_socket, 10) < 0) { // socketfd and backlog (# requests kept waiting)
            perror("server: listen");
            exit(1);
        }
        
        // 4. Accept clients
        if ((new_socket = accept(create_socket, (struct sockaddr *) &address, &addrlen)) < 0) {
            perror("server: accept");
            exit(1);
        }
        
        if (new_socket > 0) {
            printf("The client is connected.\n");
        }
        
        // 5. Send and receive data
        
        // clear buffer
        memset(buffer, 0, sizeof(buffer));
        
        recv(new_socket, buffer, bufsize, 0);
        printf("%s\n", buffer);
        write(new_socket, "hello world!\n", 12);
        close(new_socket);
    }
    close(create_socket);
    return 0;
}
