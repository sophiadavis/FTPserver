#include "ftpServer.h"
#include "response.h"
#include "socketConnection.h"

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
    
    printf("initial message sent\n");
    
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
            *listening_data_socket = open_and_bind_socket_on_port(*data_port);
            
            // Client expects (a1,a2,a3,a4,p1,p2), where port = p1*256 + p2
            int p1 = *data_port / 256;
            int p2 = *data_port % 256;
            printf("opened new port\n");
            
            int listen_status = listen(*listening_data_socket, 10);
            check_status(listen_status, "listen");
            printf("listening\n");
            
            // Tell client where to listen
            snprintf(response, MAX_MSG_LENGTH, "%s =127,0,0,1,%i,%i\n", "227 Entering Passive Mode", p1, p2);
            bytes_sent += send(new_socket, response, strlen(response), 0);
            already_sent = 1;
            
            printf("sent response\n");
            
            // Accept clients
            struct sockaddr_storage client;
            socklen_t addr_size = sizeof(client);
        
            *accept_data_socket = accept(*listening_data_socket, (struct sockaddr *) &client, &addr_size);
            check_status(*accept_data_socket, "accept");
            printf("accepted!\n");
            
//             freeaddrinfo(data_results);
            printf("freed!\n");
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

unsigned long getFileLength(FILE *fp) {
    fseek(fp, 0, SEEK_END);
    unsigned long length = ftell(fp);
    return length;
}
