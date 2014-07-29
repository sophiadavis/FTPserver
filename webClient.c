#include<netinet/in.h>    
#include<stdio.h>    
#include<stdlib.h>
#include<string.h>    
#include<sys/socket.h>    
#include<sys/stat.h>    
#include<sys/types.h>    
#include<unistd.h>  
#include <netdb.h>
#include <errno.h>


int main(int argc, char *argv[]) {

    struct addrinfo address;
    memset(&address, 0, sizeof(address)); // clear memory for struct
    address.ai_family = AF_INET;
    address.ai_socktype = SOCK_STREAM;
    address.ai_protocol = 0;
    //address.ai_flags = AI_PASSIVE;
    
    struct addrinfo *results; // pointer to results

    int status = getaddrinfo("localhost", "8000", &address, &results);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }
    printf("status: %i\n", status);
    
	
    // loop through all the results and connect to the first we can
    int sock = socket(results->ai_family, results->ai_socktype, results->ai_protocol);

    struct addrinfo *p;
    for(p = results; p != NULL; p = p->ai_next) {
        printf("-socket: %d\n", sock);
        if ((sock = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        if (connect(sock, p->ai_addr, p->ai_addrlen) == -1) {
            close(sock);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }
    
    printf("\nConnected to server.\n");
    printf("Sending data to server...\n");
    
    char *msg = "hi server!\n";
    int len, bytes_sent;
    len = strlen(msg);
    bytes_sent = send(sock, msg, len, 0);

    int bufsize = 1024;
    char *buffer = malloc(bufsize);
    recv(sock, buffer, bufsize-1, 0);

    printf("\nReceived: \n\t%s\n", buffer);
    
    freeaddrinfo(results);
    return 0;
}