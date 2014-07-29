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
    address.ai_flags = AI_PASSIVE;
    
    struct addrinfo *results; // pointer to results

    // converts host name to ip address
    int status = getaddrinfo("localhost", "8000", &address, &results);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }
    printf("status: %i\n", status);
    
	
    // loop through all the results and connect to the first we can
    int sockfd = socket(results->ai_family, results->ai_socktype, results->ai_protocol);

    struct addrinfo *p;
    for(p = results; p != NULL; p = p->ai_next) {
        printf("-\n");
        if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
            perror("client: socket");
            continue;
        }
        printf("socket: %d\n", sockfd);

        if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
            close(sockfd);
            perror("client: connect");
            continue;
        }

        break;
    }

    if (p == NULL) {
        fprintf(stderr, "client: failed to connect\n");
        return 2;
    }

//     int sock = socket(results->ai_family, results->ai_socktype, results->ai_protocol);
//         
//     int connection = connect(sock, results->ai_addr, results->ai_addrlen);
//     
//     printf("socket %i\n", sock);
//     printf("did it work? %i\n", connection);
//     
//     int bufsize = 1024;
//     char *buffer = malloc(bufsize);
//     memset(buffer, 0, bufsize); // Clear buffer
//         
//     recv(sock, buffer, bufsize, 0);
//     printf("The client's buffer: %s\n", buffer);
    
    freeaddrinfo(results);
    return 0;
}