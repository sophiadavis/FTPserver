#include<netinet/in.h>    
#include<stdio.h>    
#include<stdlib.h>
#include<string.h>    
#include<sys/socket.h>    
#include<sys/stat.h>    
#include<sys/types.h>    
#include<unistd.h>  
#include <netdb.h>
#include <dirent.h>

#include <pthread.h>

void* process_connection(void *sock);
int process_request(char *buffer, int new_socket, int bytes_received, int *signed_in);
int sign_in_thread(char *username);
int pwd(char *cwd, char *data, size_t cwd_size);
int prepare_socket(int port, struct addrinfo *results);
void check_status(int status, const char *error);

// Commands
const char *USER = "USER";
        // USER <SP> <username> <CRLF>
        // Login as user username. 
        // If the username is "anonymous", reply with 230. Otherwise, reply with 530.
const char *QUIT = "QUIT"; 
        // QUIT <CRLF>
        // Quit
        // Return 221. The server now knows that nobody is logged in.
const char *PWD = "PWD";  // "pwd";//
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
const char *SYST = "SYST";
const char *FEAT = "FEAT";
const char *PASV = "PASV";
        
int num_threads = 0;
int MAIN_PORT = 5000;
const char *ROOT = "/var/folders/r6/mzb0s9jd1639123lkcsv4mf00000gn/T/server";

int main(int argc, char *argv[]){
    
    // Set directory of server
    int cwd_success = chdir(ROOT);
    if (cwd_success < 0) {
        perror("server: CSD");
        exit(1);
    }
    
    // For storing results from creating socket
    struct addrinfo *results;
    
    // Create and bind socket to specified port
    int listening_socket = prepare_socket(MAIN_PORT, results);
    
    // Listen for connections
    pid_t pID;
    int new_socket;
    while (1) {
    
        int listen_status = listen(listening_socket, 10);
        check_status(listen_status, "listen");
    
    // Accept clients
    
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
    
    // Initial connection
    char *initial_message;
    initial_message = "220 Sophia's FTP server (Version 0.0) ready.\n";
    send(new_socket, initial_message, strlen(initial_message), 0);
        
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
    size_t data_size = 1024*sizeof(char);
    char *data = malloc(data_size);
    memset(data, 0, data_size);
    
    int len, bytes_sent, num_args;
        
    // all commands contain less than two words (otherwise, error)
    num_args = 2;
    char parsed[num_args][20];
    
    printf("\n------------------------------------------\n");
    printf("\nServer received: %s (%i bytes)\n", buffer, bytes_received);
    len = strlen(buffer);
    size_t i = 0;
    size_t j = 0;
    size_t arg_len = 0;

    while (j < num_args) {
        memset(parsed[j], '\0', 20);
        char * pch;
        printf ("Splitting buffer into tokens: %s\n", buffer);
        pch = strtok(buffer," \t\n\r");
        while (pch != NULL) {
            snprintf(parsed[j], 20, "%s", pch);
            pch = strtok(NULL, " \t\n\r");
            j++;
        }
    }
    
    int z = 0;
    while(z < num_args) {
        printf("command %zd is %s\n", z, parsed[z]);
        int a = 0;
        for(a = 0; a < 20; a++) {
            if (parsed[z][a] == '\0') {
                printf("NULL-");
            }
            else {
              printf("%c-", parsed[z][a]);
            }
        }
        z++;
        printf("\n");
    }
    
    printf("********\n");
    printf("%s (%zd), %s (%zd)\n", parsed[0], strlen(parsed[0]), parsed[1], strlen(parsed[1]));
    
    printf("********\n");
    
    if (strcmp(parsed[0], USER) == 0) {
        *sign_in_status = sign_in_thread(parsed[1]);
        if (*sign_in_status == 1) {
            snprintf(data, data_size, "%s", "230 User signed in. Using binary mode to transfer files.\n");
        }
        else {
            snprintf(data, data_size, "%s", "530 Sign in failure.\n");
        }
    }
    else if (strcmp(parsed[0], QUIT) == 0) {
        num_threads--;
        printf("Closing connection.\n");
        return 0;
    }
    else if (*sign_in_status == 1) {
        if (strcmp(parsed[0], PWD) == 0) {
            
            char *cwd = malloc(data_size);
            int pwd_status = pwd(cwd, data, data_size);
            free(cwd);
        }
        else if (strcmp(parsed[0], CWD) == 0) {
            ///////////////////////////////////////////
            // TODO keep client from going above root????
            ///////////////////////////////////////////
            // ALSO, THREADS SHARE WORKING DIRECTORY, DAMMIT.
            ///////////////////////////////////////////
            
            int chdir_status = chdir(parsed[1]);
            if (chdir_status == 0) {
                snprintf(data, data_size, "%s", "250 CWD successful\n");
            }
            else {
                perror("CWD");
                snprintf(data, data_size, "%s", "550 CWD error\n");
            }
        }
//         else if (strcmp(parsed[0], PORT) == 0) {
//             data = "you want the port number";
//         }
        else if (strcmp(parsed[0], NLST) == 0) {
            // http://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
            int bytes_written = 0;
            DIR *d;
            struct dirent *dir;
            d = opendir(".");
            if (d) {
              while ((dir = readdir(d)) != NULL) {
                bytes_written = bytes_written + snprintf(data + bytes_written, data_size, "\n%s", dir->d_name);
              }
              closedir(d);
            }
            else {
                snprintf(data, data_size, "%s", "550 NLST error\n");
            }
        }
        else if (strcmp(parsed[0], RETR) == 0) {
//             
//             FILE *fp;
//             fp = fopen(parsed[1], "r");
//     
//             if (fp == NULL) {
//               fprintf(stderr, "Can't open file!\n");
//               exit(1);
//             }
//     
//             char fileBuf[1000];
//             char myFile[10000];
//             strncpy(myFile,"FILE: \n", 7);
//             int numCharsAllotted = 1;
//             while (fgets(fileBuf, 1000, fp) != NULL) { // while we haven't reached EOF
//                 strncat(myFile, fileBuf, numCharsAllotted*1000);
//                 numCharsAllotted++;
//             }
//             printf("%s", myString);
//     
//             fclose(fp);
            
            snprintf(data, data_size, "%s", "You want to RETR\n");
        }
        else if (strcmp(parsed[0], TYPE) == 0) {
            snprintf(data, data_size, "%s", "You want the Type\n");
        }
        else if (strcmp(parsed[0], SYST) == 0) {
            snprintf(data, data_size, "%s", "215 MACOS Sophia's Server\n");
        }
        else if (strcmp(parsed[0], FEAT) == 0) {
            snprintf(data, data_size, "%s", "211 end\n");
        }
        // 200 host and port address
        else if (strcmp(parsed[0], PASV) == 0) {
            snprintf(data, data_size, "%s\n", "227 Entering Passive Mode (127,0,0,1,50,00)");
        }
        else {
            snprintf(data, data_size, "%s", "500 Syntax error, command unrecognized.\n");
        }
    }
    else {
        snprintf(data, data_size, "%s", "530 User not logged in.\n");
    }
    
    bytes_sent = send(new_socket, data, strlen(data), 0);
    printf("Data sent: %s\n", data);
    
    // Clear out parsed arguments
//     for(z = 0; z < num_args; z++) {
//         memset(parsed[z], 0, strlen(parsed[z]));
//     }
    
    free(data);
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

// Copies working directory into "data," along with appropriate success code
int pwd(char *cwd, char *data, size_t cwd_size) {
    if (getcwd(cwd, cwd_size) != NULL) {
        printf("257 Current working directory is: %s\n", cwd);
        snprintf(data, cwd_size, "257 \"%s\" \n", cwd);
//         snprintf(data, cwd_size, "257 Current working directory is: '/'\n");
        return 1;
    }
    else {
        perror("getcwd() error");
        return -1;
        exit(1);
    }
}

int prepare_socket(int port, struct addrinfo *results) {

    int listening_socket;
    
    // Socket address information:
    struct sockaddr_in address_in; // sockaddr_in contains IPv4 information 
    address_in.sin_family = AF_INET; // IPv4
    address_in.sin_port = htons(port);
    address_in.sin_addr.s_addr = INADDR_ANY; // expects 4-byte IP address (INADDR_ANY = use my IPv4 address)
        
    struct addrinfo address; // addrinfo contains info about the socket
    memset(&address, 0, sizeof(address));
    address.ai_socktype = SOCK_STREAM;
    address.ai_protocol = 0; // 0 = choose correct protocol for stream vs datagram
    address.ai_addr = (struct sockaddr *) &address_in;
    address.ai_flags = AI_PASSIVE; // fills in IP automatically
    
    char port_str[5];
    sprintf(port_str, "%d", port);
     
    int status = getaddrinfo(NULL, port_str, &address, &results); // results stored in results
    char message[50];
    sprintf(message, "getaddrinfo error: %s\n", gai_strerror(status));
    check_status(status, message);
    
    // Create socket
    listening_socket = socket(results->ai_family, results->ai_socktype, results->ai_protocol); 
    if (listening_socket > 0) {
        printf("Socket created, listening on port %s.\n", port_str);
    }
    
    // Allow reuse of port -- from Beej
    int yes = 1;
    int sockopt_status = setsockopt(listening_socket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));
    check_status(sockopt_status, "setsockopt");
    
    // Bind socket to address
    // socket id, *sockaddr struct w address info, length (in bytes) of address
    int bind_status = bind(listening_socket, (struct sockaddr *) &address_in, sizeof(address_in));                                                          
    check_status(bind_status, "bind");
    printf("Binding socket...\n");
    
    return listening_socket;
}

void check_status(int status, const char *error) {
    if (status < 0) {
        char message[100];
        sprintf(message, "server: %s", error);
        perror(message);
        exit(1);
    }
}


