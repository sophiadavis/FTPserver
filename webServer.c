#include<netinet/in.h>    
#include<stdio.h>    
#include<stdlib.h>
#include<string.h>    
#include<sys/socket.h>    
#include<sys/stat.h>    
#include<sys/types.h>    
#include<unistd.h>  
#include <netdb.h>

#include <pthread.h>

void* process_connection(void *sock);
int process_request(char *buffer, int new_socket, int bytes_received, int *signed_in);
int sign_in_thread(char *username);
char *pwd();

// Commands
const char *USER = "USER";
        // USER <SP> <username> <CRLF>
        // Login as user username. 
        // If the username is "anonymous", reply with 230. Otherwise, reply with 530.
const char *QUIT = "QUIT"; 
        // QUIT <CRLF>
        // Quit
        // Return 221. The server now knows that nobody is logged in.
const char *PWD = "PWD"; 
        //PWD <CRLF>
        // Print working directory.
        // Reply with 257 (and include the working directory as the string following the reply number).
const char *CWD = "CWD"; 
        // CWD <SP> <pathname> <CRLF>
        // Change working directory.
        // You may assume that pathname will be a pathname that is either ".." or "." or a relative pathname that does not include ".." or ".". 
        // If the requested action can be successfully completed, reply with 250. Otherwise, reply with 550.
const char *PORT = "PORT";
        // PORT <SP> <host-port> <CRLF>
        //Specifies the port to be used for the data connection.
        // READ MORE Create a new socket (for the data connection) given the host address and the port number. Reply with 200 to indicate success. Reply with 500 to indicate failure.
const char *NLST = "NLST";
        // NLST [<SP> <pathname>] <CRLF>
        // List files in the current directory or files specified by the optional arguments.
        // First, indicate that a data communication transfer is starting with reply 125. Next, transfer the file names over the data communication channel that was previously opened by the client having issued a PORT command. (You may assume that the client will have issued the PORT command.) Once the action is successfully completed, reply with 226. In case of error, reply with 450. In both cases close the data communication channel upon completion.

const char *RETR = "RETR";
        // RETR <SP> <pathname> <CRLF>
        // Retrieve a file specified by path pathname.
        // You should assume that the client has called PORT to initialize the data communication channel. If no file has been specified for retrieval, reply with 450. If a file has been specified, first send reply 125. Next, if the specified file exists, is a file, and is readable, send the file byte by byte over using the data communication. If this action is completed successfully, reply with 250. Otherwise, reply with 550. In both cases, close the data communication channel upon completion.

const char *TYPE = "TYPE";
        // TYPE <Transfer mode> <CRLF>
        // simply reply with 200
        
int num_threads = 0;
const char *ROOT = "/var/folders/r6/mzb0s9jd1639123lkcsv4mf00000gn/T/server";

int main(int argc, char *argv[]){

    int cwd_success = chdir(ROOT);
    if (cwd_success < 0) {
        perror("server: CSD");
        exit(1);
    }
    
    int listening_socket, new_socket;
    
    // Socket address information
        // sockaddr_in contains IPv4 information
        struct sockaddr_in address_in; 
        address_in.sin_family = AF_INET;
        address_in.sin_port = htons(5000);
        address_in.sin_addr.s_addr = INADDR_ANY;
            
        // addrinfo contains info about the socket
        struct addrinfo address;
        memset(&address, 0, sizeof(address));
        address.ai_socktype = SOCK_STREAM;
        address.ai_protocol = 0;
        address.ai_addr = (struct sockaddr *) &address_in;
        address.ai_flags = AI_PASSIVE;
    
        // For storing results from getaddrinfo
        struct addrinfo *results;
     
    int status = getaddrinfo(NULL, "5000", &address, &results);
    if (status != 0) {
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(1);
    }
    
// 1. Create socket
    listening_socket = socket(results->ai_family, results->ai_socktype, results->ai_protocol); // IPv4, stream, 0 = choose correct protocol for stream vs datagram 
    if (listening_socket > 0) {
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
        perror("server: bind");
        exit(1);
    }
    
// 3. Listen for connections
    pid_t pID;
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
            pthread_t tid;
            num_threads++;
            pthread_create(&tid, NULL, &process_connection, &new_socket);
            printf("\nA client has connected, new thread created. Total threads: %d.\n", num_threads);
            
//             /// WHERE SHOULD THIS GO???
//             // thread has completed work
//             int *total_bytes_sent;
//             int join_status = pthread_join(tid, (void **) &total_bytes_sent);
//             
//             if (join_status == 0) {
//                 printf("Thread closed. %d total bytes sent.\n", *total_bytes_sent);
//             }
//             else {
//                 printf("Error joining thread\n");
//             }
        }
    }
    freeaddrinfo(results);
    close(new_socket);
    close(listening_socket);
    return 0;
}

void *process_connection(void *sock) {
    int *new_socket_ptr;
    int new_socket;
    int signed_in = 0;
    
    // cast void* as int*
    new_socket_ptr = (int *) sock;
    
    // get int from int*
    new_socket = *new_socket_ptr;
        
// 5. Send and receive data
    int bufsize = 1024;
    int *total_bytes_sent = malloc(sizeof(int));
    *total_bytes_sent = 0;
    char *buffer = malloc(bufsize);
    int bytes_received, bytes_sent;
    
    while (1) {
        memset(buffer, 0, bufsize);
        bytes_received = recv(new_socket, buffer, bufsize, 0);
        if (bytes_received > 0) {
            bytes_sent  = process_request(buffer, new_socket, bytes_received, &signed_in);
        }
        
        // TODO -- error checking?
        if (bytes_sent < 0) {
            close(new_socket);
            printf("?????Server closed connection.\n");
            exit(0);
        }
        
        // Client has terminated.
        else if (bytes_sent == 0) {
            close(new_socket);
            printf("Client closed connection.\n");
            break;
        }
        else {
            *total_bytes_sent += bytes_sent;
        }
    }
    free(buffer);
    printf("Thread closed. %d total bytes sent. Threads still active: %d.\n", *total_bytes_sent, num_threads);
    free(total_bytes_sent);
    pthread_exit((void *) total_bytes_sent);
}
/*    END PROCESS CONNECTION    */

int process_request(char *buffer, int new_socket, int bytes_received, int *sign_in_status) {
    char *data;
    int len, bytes_sent, num_args;
    
    int blatant_memory_leak = 0;
    
    // all commands contain less than two words (otherwise, error)
    num_args = 2;
    char parsed[num_args][20];// = memset(parsed[2][20], 0, sizeof(parsed[2][20])); // TODO -- is 20 adequate?
    
    printf("\nServer received: %s (%i bytes)\n", buffer, bytes_received);
    len = strlen(buffer);
    size_t i = 0;
    size_t j = 0;
    size_t arg_len = 0;
    
    while (j < num_args) {
        while (i < len) {
            if (buffer[i] != ' ') {
                parsed[j][arg_len] = buffer[i];
                arg_len++;
            }
            else {
                j++;
                arg_len = 0; // Reset length of current arg
            }
            i++;
        }
        break;
    }
    
    int z = 0;
    for(z = 0; z < num_args; z++) {
        printf("command %zd is %s\n", z, parsed[z]);
    }
        
    if (strcmp(parsed[0], USER) == 0) {
        *sign_in_status = sign_in_thread(parsed[1]);
        if (*sign_in_status == 1) {
//             strcpy(data, "230-User signed in.");
            data = "230-User signed in.";
        }
        else {
//             strcpy(data, "530-Sign in failure.");
            data = "530-Sign in failure.";
        }
    }
    else if (strcmp(parsed[0], QUIT) == 0) {
        num_threads--;
        printf("quitting.\n");
        return 0;
    }
    else if (*sign_in_status == 1) {
        if (strcmp(parsed[0], PWD) == 0) {
            printf("you want to pwd\n");
            printf("data is %s\n", data);
            
            
            size_t cwd_size = 1024*sizeof(char);
            char *cwd = malloc(cwd_size);
            data = malloc(cwd_size); // BLATANT MEMORY LEAK -- see hack fix later
            blatant_memory_leak = 1;
            if (getcwd(cwd, cwd_size) != NULL) {
                printf("257-Current working directory is: %s\n", cwd);
                snprintf(data, cwd_size, "257-Current working directory is: %s\n", cwd);
            }
            else {
                perror("getcwd() error");
                exit(1);
            }
            printf("data is %s\n", data);
            free(cwd);
        }
        else if (strcmp(parsed[0], CWD) == 0) {
            ///////////////////////////////////////////
            // TODO keep client from going above root????
            ///////////////////////////////////////////
            int chdir_status = chdir(parsed[1]);
            if (chdir_status == 0) {
                data = "250-CWD successful";
            }
            else {
                perror("CWD");
                data = "550-CWD error";
            }
        }
//         else if (strcmp(parsed[0], PORT) == 0) {
//             data = "you want the port number";
//         }
        else if (strcmp(parsed[0], NLST) == 0) {
            // LS.
            data = "you want the nlst??";
        }
        else if (strcmp(parsed[0], RETR) == 0) {
            data = "retr?? wtf";
        }
        else if (strcmp(parsed[0], TYPE) == 0) {
            data = "you want the type";
        }
        else {
            data = "500-Syntax error, command unrecognized.";
        }
    }
    else {
        data = "530-User not logged in.";
    }
    
    bytes_sent = send(new_socket, data, strlen(data), 0);
    
    // Clear out parsed arguments
    for(z = 0; z < num_args; z++) {
        memset(parsed[z], 0, strlen(parsed[z]));
    }
    
//     free(data);
    if (blatant_memory_leak == 1) {
        free(data);
    }
    return bytes_sent;     
}
/*    END PROCESS REQUEST    */

int sign_in_thread(char *username) {
    if (strcmp(username, "anonymous") == 0) {
        return 1;
    }
    else {
        return -1;
    }
}
