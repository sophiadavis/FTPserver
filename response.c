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

    Connection client;
    
    // Bookkeeping for this thread
    client.sign_in_status = 0;
    client.data_port = CURRENT_CONNECTION_PORT;
    client.listening_data_socket = 0;
    client.accept_data_socket = 0;
    
    // Cast void* as int*
    int *new_socket_ptr = (int *) sock;
    client.main_socket = *new_socket_ptr;
        
    // Prepare to send and receive data
    char *buffer = malloc(MAX_MSG_LENGTH);
    
    int bytes_sent = send_formatted_response_to_socket(client.main_socket, 220, "Sophia's FTP server (Version 0.0) ready.");
    client.total_bytes_sent = bytes_sent;
    
    int bytes_received; 
    while (1) {
        memset(buffer, '\0', MAX_MSG_LENGTH);
        bytes_received = recv(client.main_socket, buffer, MAX_MSG_LENGTH, 0);
        check_status(bytes_received, "receive");
        
        if (bytes_received > 0) {
            bytes_sent  = process_request(buffer, &client, bytes_received);
            check_status(bytes_sent, "send");
            client.total_bytes_sent += bytes_sent;
        }
        else { // bytes_recv == 0
            close(client.main_socket);
            printf("Client closed connection.\n");
            break;
        }
    }
    printf("Thread closed. %d total bytes sent. Threads still active: %d.\n", client.total_bytes_sent, NUM_THREADS);
    free(buffer);
    pthread_exit((void *) &(client.total_bytes_sent));
}
/*    END PROCESS CONTROL CONNECTION    */

// Handles FTP client requests and sends appropriate responses
int process_request(char *buffer, Connection* client, int bytes_received) {

    int bytes_sent = 0;
    
    int already_sent = 0;
        
    // Assuming all commands contain less than two words (TODO: otherwise, error??)
    MAX_NUM_ARGS = 2;
    char parsed[MAX_NUM_ARGS][MAX_COMMAND_LENGTH];
    
    printf("\n------------------------------------------\n");
    printf("\nServer received: %s (%i bytes)\n", buffer, bytes_received);

    // Parse buffer, splitting on spaces, tabs, nl, cr
    int j = 0;
    char* pch;
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
        client->sign_in_status = sign_in_thread(parsed[1]);
        if (client->sign_in_status == 1) {
            bytes_sent += send_formatted_response_to_socket(client->main_socket, 230, "User signed in. Using binary mode to transfer files.");
        }
        else {
            bytes_sent += send_formatted_response_to_socket(client->main_socket, 530, "Sign in failure");
        }
    }
    else if (strcmp(parsed[0], QUIT) == 0) {
        NUM_THREADS--;
        bytes_sent += send_formatted_response_to_socket(client->main_socket, 221, "Goodbye.");
    }
    else if (client->sign_in_status == 1) {
        if (strcmp(parsed[0], PWD) == 0) {
            char *response = malloc(MAX_MSG_LENGTH); 
            char *cwd = malloc(MAX_MSG_LENGTH);
            if (getcwd(cwd, MAX_MSG_LENGTH) != NULL) {
                printf("%s\n", cwd);
                snprintf(response, MAX_MSG_LENGTH, "\"%s\"", cwd);
            }
            else {
                perror("getcwd() error");
                return -1;
                exit(1);
            }

//             int pwd_status = pwd(cwd, response, MAX_MSG_LENGTH);
            
            bytes_sent += send_formatted_response_to_socket(client->main_socket, 257, response);
            free(cwd);
        }
        else if (strcmp(parsed[0], CWD) == 0) {
            ///////////////////////////////////////////
            // THREADS SHARE WORKING DIRECTORY, DAMMIT.
            ///////////////////////////////////////////
            
            int chdir_status = chdir(parsed[1]);
            if (chdir_status == 0) {
                bytes_sent += send_formatted_response_to_socket(client->main_socket, 250, "CWD successful.");
            }
            else {
                perror("CWD");
                bytes_sent += send_formatted_response_to_socket(client->main_socket, 550, "CWD error.");
            }
        }
        else if (strcmp(parsed[0], PASV) == 0) {
            CURRENT_CONNECTION_PORT++;
            client->data_port = CURRENT_CONNECTION_PORT;
            client->listening_data_socket = open_and_bind_socket_on_port(client->data_port);
            
            // Client expects (a1,a2,a3,a4,p1,p2), where port = p1*256 + p2
            int p1 = client->data_port / 256;
            int p2 = client->data_port % 256;
            printf("opened new port\n");
            
            int listen_status = listen(client->listening_data_socket, BACKLOG);
            check_status(listen_status, "listen");
            printf("listening\n");
            
            // Tell client where to listen
            char *response = malloc(MAX_MSG_LENGTH);
            snprintf(response, MAX_MSG_LENGTH, "%s =127,0,0,1,%i,%i", "Entering Passive Mode", p1, p2);
            bytes_sent += send_formatted_response_to_socket(client->main_socket, 227, response);
            
            printf("sent response\n");

            client->accept_data_socket = open_socket_for_incoming_connection(client->listening_data_socket);
        }
        else if (strcmp(parsed[0], NLST) == 0) {
            int data_length = 0;
            int response_length = 0;
            DIR *d;
            struct dirent *dir;
            d = opendir(".");
            char dirInfo[MAX_MSG_LENGTH];
            bytes_sent += send_formatted_response_to_socket(client->main_socket, 150, "Here comes the directory listing.");
            
            if (d && client->accept_data_socket > 0) {
                while ((dir = readdir(d)) != NULL) {
                    data_length = data_length + snprintf(dirInfo + data_length, MAX_MSG_LENGTH - data_length, "%s \r\n", dir->d_name);
                }
                closedir(d);
                
                // Send directory information immediately over data socket, then close connection
                bytes_sent += send(client->accept_data_socket, dirInfo, strlen(dirInfo), 0);
                
                close(client->accept_data_socket);
                client->accept_data_socket = 0;
                client->listening_data_socket = 0;
                
                bytes_sent += send_formatted_response_to_socket(client->main_socket, 226, "Directory send OK.");
            }
            else {
                bytes_sent += send_formatted_response_to_socket(client->main_socket, 550, "NLST error.");
            }
        }
        else if (strcmp(parsed[0], RETR) == 0) {
            
            FILE *fp;
            fp = fopen(parsed[1], "rb");
            
            bytes_sent += send_formatted_response_to_socket(client->main_socket, 150, "Opening binary mode data connection.");
            if ((fp != NULL) && client->accept_data_socket > 0) {
                              
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
                    bytes_sent += send(client->accept_data_socket, (const void *) &fileBuffer[i], 1, 0);
                }
                fclose(fp);

                // Data has been sent, now close connection
                close(client->accept_data_socket);
                client->accept_data_socket = 0;
                client->listening_data_socket = 0;
                
                bytes_sent += send_formatted_response_to_socket(client->main_socket, 226, "Transfer complete.");
                
            }
            else {
                bytes_sent += send_formatted_response_to_socket(client->main_socket, 550, "RETR error.");
            }
        }
        else if (strcmp(parsed[0], TYPE) == 0) {
            bytes_sent += send_formatted_response_to_socket(client->main_socket, 200, "Using ASCII mode for directory listings and binary mode to transfer files.");
        }
        else if (strcmp(parsed[0], SYST) == 0) {
            bytes_sent += send_formatted_response_to_socket(client->main_socket, 215, "MACOS Sophia's Server.");
        }
        else if (strcmp(parsed[0], FEAT) == 0) {
            bytes_sent += send_formatted_response_to_socket(client->main_socket, 211, "end");
        }
        else {
            bytes_sent += send_formatted_response_to_socket(client->main_socket, 502, "Command not implemented.");
        }
    }
    else {
        bytes_sent += send_formatted_response_to_socket(client->main_socket, 530, "User not logged in.");
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

int send_formatted_response_to_socket(int socket, int code, const char* response) {
    char *message = malloc(MAX_MSG_LENGTH);
    memset(message, '\0', MAX_MSG_LENGTH);
    
    snprintf(message, MAX_MSG_LENGTH, "%d %s\r\n", code, response);
    
    int bytes_sent = send(socket, message, strlen(message), 0);
    
    printf("Data sent: %s\n", message);
    free(message);
    
    return bytes_sent;
}
    

// Copies working directory into "response," along with appropriate success code
// int pwd(char *cwd, char *response, size_t cwd_size) {
//     if (getcwd(cwd, cwd_size) != NULL) {
//         snprintf(response, cwd_size, "257 \"%s\" \n", cwd);
//         return 1;
//     }
//     else {
//         perror("getcwd() error");
//         return -1;
//         exit(1);
//     }
// }

unsigned long getFileLength(FILE *fp) {
    fseek(fp, 0, SEEK_END);
    unsigned long length = ftell(fp);
    return length;
}
