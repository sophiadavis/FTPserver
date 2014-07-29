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
    int status = getaddrinfo("localhost", "15000", &address, &results);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }
    printf("status: %i\n", status);
    
    struct addrinfo *p;
//     for(p = results; p != NULL; p = p->ai_next) {
//         printf("-\n");
//         printf(" canonname: %s\n", p->ai_canonname);
//         printf(" socktype: %i\n", p->ai_socktype);
//         printf(" family: %i\n", p->ai_family);
//     }

    struct sockaddr_in address_in; 
    address_in.sin_family = AF_INET;
    address_in.sin_port = htons(8000);
    address_in.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
    
    char str[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &(address_in.sin_addr), str, INET_ADDRSTRLEN);
    
    printf("%s\n", str);
	
    // loop through all the results and connect to the first we can
    int sockfd = socket(address_in.sin_family, SOCK_STREAM, 0);
    if (connect(sockfd, (struct sockaddr *) &address_in, (socklen_t) sizeof(address_in)) == -1) {
            close(sockfd);
            perror("client: connect");
            printf("%d\n", errno);
        }
//     for(p = results; p != NULL; p = p->ai_next) {
//         printf("-\n");
//         if ((sockfd = socket(p->ai_family, p->ai_socktype, p->ai_protocol)) == -1) {
//             perror("client: socket");
//             continue;
//         }
//         printf("socket: %d\n", sockfd);
// 
//         if (connect(sockfd, p->ai_addr, p->ai_addrlen) == -1) {
//             close(sockfd);
//             perror("client: connect");
//             continue;
//         }
// 
//         break;
//     }

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