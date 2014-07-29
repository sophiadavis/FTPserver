#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h> 
#include <netdb.h>


int main(int argc, char *argv[]) {

    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints)); // clear memory for struct
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    hints.ai_protocol = 0;
    hints.ai_flags = AI_PASSIVE;
    
    struct addrinfo *res; // pointer to results

    // converts host name to ip address
    int status = getaddrinfo("localhost", "15000", &hints, &res);
    printf("status: %i\n", status);
    
    struct addrinfo *p;
    for(p = res; p != NULL; p = p->ai_next) {
        printf("-\n");
        printf(" canonname: %s\n", p->ai_canonname);
        printf(" socktype: %i\n", p->ai_socktype);
        printf(" family: %i\n", p->ai_family);
    }

    int sock = socket(res->ai_family, res->ai_socktype, res->ai_protocol);
    
    int connection = connect(sock, res->ai_addr, res->ai_addrlen);
    
    printf("socket %i\n", sock);
    printf("did it work? %i\n", connection);
    
    
    //struct *sockaddr;
    
    //int connect(int socket, struct sockaddr *serv_addr, int addrlen); 
    
    freeaddrinfo(res);
    return 0;
}