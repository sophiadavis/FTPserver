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
    
    struct addrinfo *results; // pointer to results

    int status = getaddrinfo("localhost", "5000", &address, &results);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(status));
        return 1;
    }
    printf("Status: %i\n", status);
	
    // loop through all the results and connect to the first we can
    int sock = socket(results->ai_family, results->ai_socktype, results->ai_protocol);

    struct addrinfo *p;
    for(p = results; p != NULL; p = p->ai_next) {
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
    
    int bufsize = 1024;
    int commandsize = 20*sizeof(char);
    char *buffer = malloc(bufsize);
    char *command = malloc(commandsize);
    int len, bytes_sent, bytes_received;
    while (1) {
//         printf("--Client starting while loop.--\n");
        memset(buffer, 0, bufsize); // Clear buffer
        memset(command, 0, commandsize); // Clear command
        printf("> ");
        int scanned = scanf("%[^\t\n]s", command);
        getchar(); // eat newline

        if (strlen(command) > 0) { //--scanf to accept multi-word string
            len = strlen(command);
            printf("\tSending to server: '%s' (%d bytes)\n", command, len);
        
            bytes_sent = send(sock, command, len, 0);
            if (bytes_sent < 0) {
                close(sock);
                printf("Server closed connection.\n");
                perror("server: connect");
                exit(0);
            }
            bytes_received = recv(sock, buffer, bufsize-1, 0);

            printf("\tServer replied: '%s' (%d bytes)\n", buffer, bytes_received);
        }
        else {
            close(sock);
            printf("Client closed connection.\n");
            exit(0);
        }
    }
    free(buffer);
    freeaddrinfo(results);
    return 0;
}