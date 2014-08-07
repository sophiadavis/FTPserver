#include <netinet/in.h>    
#include <stdio.h>    
#include <stdlib.h>
#include <string.h>    
#include <sys/socket.h>    
#include <sys/stat.h>    
#include <sys/types.h>    
#include <unistd.h>  
#include <netdb.h>
#include <dirent.h>

#include <pthread.h>

void* process_control_connection(void *sock);
int process_request(char *buffer, int new_socket, int bytes_received, int *signed_in, int *data_port, int *listening_data_socket, int *accept_data_socket);
int sign_in_thread(char *username);
int pwd(char *cwd, char *response, size_t cwd_size);
int prepare_socket(int port, struct addrinfo *results);
void check_status(int status, const char *error);
int begin_connection(int listening_socket, void *on_create_function);
unsigned long getFileLength(FILE *fp);

// Commands implemented
const char *USER = "USER";
const char *QUIT = "QUIT"; 
const char *PWD = "PWD";
const char *CWD = "CWD"; 
const char *NLST = "NLST";
const char *RETR = "RETR";
const char *TYPE = "TYPE";
const char *SYST = "SYST";
const char *FEAT = "FEAT";
const char *PASV = "PASV";
       
int NUM_THREADS = 0;
int MAIN_PORT = 5000;
int CURRENT_CONNECTION_PORT = 5000;
const char *ROOT = "/var/folders/r6/mzb0s9jd1639123lkcsv4mf00000gn/T/server";

// Parameters for sending/receiving with client
int MAX_NUM_ARGS = 2;
int MAX_COMMAND_LENGTH = 50;
int MAX_MSG_LENGTH = 1024 * sizeof(char);

int main(int argc, char *argv[]){
    
    // Set root directory of server
    int chdir_status = chdir(ROOT);
    check_status(chdir_status, "chdir");
//     int chroot_status = chroot(ROOT);
//     check_status(chroot_status, "chroot");
    
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
    
        // Accept clients, spawn new thread for each connection
        new_socket = begin_connection(listening_socket, &process_control_connection);
    }
    freeaddrinfo(results);
    close(new_socket);
    close(listening_socket);
    return 0;
}

// Executed by new thread when server accepts new connection
// Receives client requests, calls process_request to parse request and send appropriate response
void *process_control_connection(void *sock) {
    
    // Bookkeeping for this thread
    int signed_in = 0;
    int data_port = CURRENT_CONNECTION_PORT;
    int listening_data_socket = 0;
    int accept_data_socket = 0;
    
    // Cast void* as int*
    int *new_socket_ptr = (int *) sock;
    
    // Get int from int*
    int new_socket = *new_socket_ptr;
        
    // Prepare to send and receive data
    int *total_bytes_sent = malloc(sizeof(int));
    char *buffer = malloc(MAX_MSG_LENGTH);
    int bytes_received;
    
    // Send message initializing connection
    char *initial_message;
    initial_message = "220 Sophia's FTP server (Version 0.0) ready.\n";
    int bytes_sent = send(new_socket, initial_message, strlen(initial_message), 0);
    *total_bytes_sent = bytes_sent;
    
    while (1) {
        memset(buffer, '\0', MAX_MSG_LENGTH);
        bytes_received = recv(new_socket, buffer, MAX_MSG_LENGTH, 0);
        check_status(bytes_received, "receive");
        
        if (bytes_received > 0) {
            bytes_sent  = process_request(buffer, new_socket, bytes_received, &signed_in, &data_port, &listening_data_socket, &accept_data_socket);
            check_status(bytes_sent, "send");
            *total_bytes_sent += bytes_sent;
        }
        else { // bytes_recv == 0
            close(new_socket);
            printf("Client closed connection.\n");
            break;
        }
    }
    printf("Thread closed. %d total bytes sent. Threads still active: %d.\n", *total_bytes_sent, NUM_THREADS);
    free(buffer);
    free(total_bytes_sent);
    pthread_exit((void *) total_bytes_sent);
}
/*    END PROCESS CONTROL CONNECTION    */

// Handles FTP client requests and sends appropriate responses
int process_request(char *buffer, int new_socket, int bytes_received, int *sign_in_status, int *data_port, int *listening_data_socket, int *accept_data_socket) {
    char *response = malloc(MAX_MSG_LENGTH);
    memset(response, '\0', MAX_MSG_LENGTH);
    
    // TODO make some of these globals
    int bytes_sent;
    
    int already_sent = 0;
        
    // Assuming all commands contain less than two words (TODO: otherwise, error??)
    MAX_NUM_ARGS = 2;
    char parsed[MAX_NUM_ARGS][MAX_COMMAND_LENGTH];
    
    printf("\n------------------------------------------\n");
    printf("\nServer received: %s (%i bytes)\n", buffer, bytes_received);

    // Parse buffer, splitting on spaces, tabs, nl, cr
    int j = 0;
    char * pch;
    pch = strtok(buffer," \t\n\r");
    while (pch != NULL && j < MAX_NUM_ARGS) {
        memset(parsed[j], '\0', MAX_COMMAND_LENGTH);
        snprintf(parsed[j], MAX_COMMAND_LENGTH, "%s", pch);
        pch = strtok(NULL, " \t\n\r");
        j++;
    }
    
    int z = 0;
    while(z < MAX_NUM_ARGS) {
        printf("command %zd is %s\n", z, parsed[z]);
        int a = 0;
        for(a = 0; a < MAX_COMMAND_LENGTH; a++) {
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
    
    // Find appropriate response based on client's commands
    if (strcmp(parsed[0], USER) == 0) {
        *sign_in_status = sign_in_thread(parsed[1]);
        if (*sign_in_status == 1) {
            snprintf(response, MAX_MSG_LENGTH, "%s", "230 User signed in. Using binary mode to transfer files.\n");
        }
        else {
            snprintf(response, MAX_MSG_LENGTH, "%s", "530 Sign in failure.\n");
        }
    }
    else if (strcmp(parsed[0], QUIT) == 0) {
        NUM_THREADS--;
        send(new_socket, "221 \n", 5, 0);
        return 0;
    }
    else if (*sign_in_status == 1) {
        if (strcmp(parsed[0], PWD) == 0) {
            
            char *cwd = malloc(MAX_MSG_LENGTH);
            int pwd_status = pwd(cwd, response, MAX_MSG_LENGTH);
            free(cwd);
        }
        else if (strcmp(parsed[0], CWD) == 0) {
            ///////////////////////////////////////////
            // THREADS SHARE WORKING DIRECTORY, DAMMIT.
            ///////////////////////////////////////////
            
            int chdir_status = chdir(parsed[1]);
            if (chdir_status == 0) {
                snprintf(response, MAX_MSG_LENGTH, "%s", "250 CWD successful\n");
            }
            else {
                perror("CWD");
                snprintf(response, MAX_MSG_LENGTH, "%s", "550 CWD error\n");
            }
        }
        else if (strcmp(parsed[0], PASV) == 0) {
            CURRENT_CONNECTION_PORT++;
            *data_port = CURRENT_CONNECTION_PORT;
            struct addrinfo *data_results;
            *listening_data_socket = prepare_socket(*data_port, data_results);
            
            // Client expects (a1,a2,a3,a4,p1,p2), where port = p1*256 + p2
            int p1 = *data_port / 256;
            int p2 = *data_port % 256;
            
            int listen_status = listen(*listening_data_socket, 10);
            check_status(listen_status, "listen");

            // Accept clients
            struct sockaddr_storage client;
            socklen_t addr_size = sizeof(client);
            
            snprintf(response, MAX_MSG_LENGTH, "%s =127,0,0,1,%i,%i\n", "227 Entering Passive Mode", p1, p2);
            bytes_sent += send(new_socket, response, strlen(response), 0);
            already_sent = 1;
        
            *accept_data_socket = accept(*listening_data_socket, (struct sockaddr *) &client, &addr_size);
            check_status(*accept_data_socket, "accept");
            
            freeaddrinfo(data_results);
        }
        else if (strcmp(parsed[0], NLST) == 0) {
            // Thanks to http://stackoverflow.com/questions/4204666/how-to-list-files-in-a-directory-in-a-c-program
            int data_length = 0;
            int response_length = 0;
            DIR *d;
            struct dirent *dir;
            d = opendir(".");
            char dirInfo[MAX_MSG_LENGTH];
            response_length = snprintf(response, MAX_MSG_LENGTH, "%s", "150 Here comes the directory listing. \r\n");
            if (d && *accept_data_socket > 0) {
                while ((dir = readdir(d)) != NULL) {
                    data_length = data_length + snprintf(dirInfo + data_length, MAX_MSG_LENGTH - data_length, "%s \r\n", dir->d_name);
                }
                closedir(d);
                
                // Send directory information immediately over data socket, then close connection
                bytes_sent += send(*accept_data_socket, dirInfo, strlen(dirInfo), 0);
                close(*accept_data_socket);
                *accept_data_socket = 0;
                
                snprintf(response + response_length, MAX_MSG_LENGTH, "226 Directory send OK.\r\n");
            }
            else {
                snprintf(response, MAX_MSG_LENGTH, "%s", "550 NLST error\n");
            }
        }
        else if (strcmp(parsed[0], RETR) == 0) {
            
            FILE *fp;
            fp = fopen(parsed[1], "rb");
            
            int response_length = 0;

            response_length = snprintf(response, MAX_MSG_LENGTH, "150 Opening binary mode data connection for %s (178 bytes). \r\n", parsed[1]);
            if ((fp != NULL) && *accept_data_socket > 0) {
                              
                unsigned long fileLength = getFileLength(fp);
                
                rewind(fp);

                unsigned char fileBuffer[fileLength + 1];
                if (!fileBuffer) {
                    fprintf(stderr, "Memory error!");
                    fclose(fp);
                    return 1;
                }

                fread(fileBuffer, sizeof(unsigned char), fileLength, fp);
                
                // Send file byte-by-byte over data socket,
                int i;
                for (i = 0; i < (fileLength + 1); i++) {
                    bytes_sent += send(*accept_data_socket, (const void *) &fileBuffer[i], 1, 0);
                }
                fclose(fp);

                // Data has been sent, now close connection
                close(*accept_data_socket);
                *accept_data_socket = 0;
                
                snprintf(response + response_length, MAX_MSG_LENGTH, "226 Transfer complete.\r\n");
                
            }
            else {
                snprintf(response, MAX_MSG_LENGTH, "%s", "550 RETR error\n");
            }
        }
        else if (strcmp(parsed[0], TYPE) == 0) {
            snprintf(response, MAX_MSG_LENGTH, "%s", "200 Using ASCII mode for NLST, and binary mode to transfer files.\n");
        }
        else if (strcmp(parsed[0], SYST) == 0) {
            snprintf(response, MAX_MSG_LENGTH, "%s", "215 MACOS Sophia's Server\n");
        }
        else if (strcmp(parsed[0], FEAT) == 0) {
            snprintf(response, MAX_MSG_LENGTH, "%s", "211 end\n");
        }
        else {
            snprintf(response, MAX_MSG_LENGTH, "%s", "502 Command not implemented.\n");
        }
    }
    else {
        snprintf(response, MAX_MSG_LENGTH, "%s", "530 User not logged in.\n");
    }
    if (already_sent == 0) {
        bytes_sent = send(new_socket, response, strlen(response), 0);
    }
    printf("Data sent: %s\n", response);
    
    free(response);
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
int pwd(char *cwd, char *response, size_t cwd_size) {
    if (getcwd(cwd, cwd_size) != NULL) {
        snprintf(response, cwd_size, "257 \"%s\" \n", cwd);
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

// Sets appropriate error message if status indicates error
void check_status(int status, const char *error) {
    if (status < 0) {
        char message[100];
        sprintf(message, "server: %s", error);
        perror(message);
        exit(1);
    }
}

// Accept connection and spawn new thread
int begin_connection(int listening_socket, void *on_create_function) {
    // Make a new socket specifically for sending/receiving data w this client
    int new_socket;
    
    // Info about incoming connection goes into sockaddr_storage struct 
    struct sockaddr_storage client;
    socklen_t addr_size = sizeof(client);
    
    new_socket = accept(listening_socket, (struct sockaddr *) &client, &addr_size);
    check_status(new_socket, "accept");

    if (new_socket > 0) {
        pthread_t tid;
        NUM_THREADS++;
        pthread_create(&tid, NULL, on_create_function, &new_socket);
        printf("\nA client has connected (socket %d), new thread created. Total threads: %d.\n", new_socket, NUM_THREADS);
    }
    return new_socket;
}

unsigned long getFileLength(FILE *fp) {
    fseek(fp, 0, SEEK_END);
    unsigned long length = ftell(fp);
    return length;
}


