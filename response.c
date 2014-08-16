/* response.c
*  Functions handling interpretation of client commands and formulation of appropriate
*  response.
*  Information about each connection is contained in a Connection struct (defined in response.h). 
*/

#include "response.h"
#include "socketConnection.h"

// Commands implemented
const int USER = 0;
const int QUIT = 1; 
const int PWD = 2;
const int CWD = 3; 
const int PASV = 4;
const int NLST = 5;
const int RETR = 6;
const int TYPE = 7;
const int SYST = 8;
const int FEAT = 9;

int translate_command(const char* command) {
    int int_command;
    
    if (strcmp(command, "USER") == 0) {
        int_command = 0;
    }
    else if (strcmp(command, "QUIT") == 0) {
        int_command = 1;
    }
    else if (strcmp(command, "PWD") == 0) {
        int_command = 2;
    }
    else if (strcmp(command, "CWD") == 0) {
        int_command = 3;
    }
    else if (strcmp(command, "PASV") == 0) {
        int_command = 4;
    }
    else if (strcmp(command, "NLST") == 0) {
        int_command = 5;
    }
    else if (strcmp(command, "RETR") == 0) {
        int_command = 6;
    }
    else if (strcmp(command, "TYPE") == 0) {
        int_command = 7;
    }
    else if (strcmp(command, "SYST") == 0) {
        int_command = 8;
    }
    else if (strcmp(command, "FEAT") == 0) {
        int_command = 9;
    }
    else {
        int_command = 1000;
    } 
    return int_command;
}

// Executed by new thread when server accepts new connection
// Receives client requests, calls process_request to parse request and send appropriate response
void *process_control_connection(void *sock) {

    Connection client;
    
    // Bookkeeping for this thread
//     client.thread_wd = malloc(MAX_MSG_LENGTH);
    set_thread_wd(&client);
    client.sign_in_status = 0;
    client.data_port = CURRENT_CONNECTION_PORT;
    client.listening_data_socket = 0;
    client.accept_data_socket = 0;
    
    // Cast void* as int*
    int *new_socket_ptr = (int *) sock;
    client.main_socket = *new_socket_ptr;
        
    // Prepare to send and receive data
    char *buffer = malloc(MAX_MSG_LENGTH);
    
    int bytes_sent = send_formatted_response_to_client(&client, 220, "Sophia's FTP server (Version 0.0) ready.");
    printf("%d", INT_MAX);
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
    free(new_socket_ptr);
//     free(client.thread_wd);
    pthread_exit((void *) &(client.total_bytes_sent));
} /*    END PROCESS CONTROL CONNECTION    */

// Handles FTP client requests and sends appropriate responses
int process_request(char *buffer, Connection* client, int bytes_received) {

    int bytes_sent = 0;
    
    printf("\n------------------------------------------\n");
    printf("\nServer received: %s (%i bytes)\n", buffer, bytes_received);
    
    // Assuming all commands contain less than two words
    char** parsed = tokenize_commands(buffer);
    
    int command = translate_command(parsed[0]);

    // Find appropriate response based on client's commands
    if (client->sign_in_status == 1 ) {
        switch(command) {
            case USER: 
                bytes_sent += process_user_command(client, parsed[1]);
                break;
        
            case PWD:
                bytes_sent += process_pwd_command(client);
                break;
        
            case CWD:
                bytes_sent += process_cwd_command(client, parsed[1]);
                break;

            case PASV:
                bytes_sent += process_pasv_command(client);
                break;
            
            case NLST:
                bytes_sent += process_nlst_command(client);
                break;

            case RETR:
                bytes_sent += process_retr_command(client, parsed[1]);
                break;
        
            case TYPE:
                bytes_sent += send_formatted_response_to_client(client, 200, "Using binary mode for data transfer.");
                break;
            
            case SYST:
                bytes_sent += send_formatted_response_to_client(client, 215, "MACOS Sophia's Server.");
                break;

            case FEAT:
                bytes_sent += send_formatted_response_to_client(client, 211, "end");
                break;
        
            case QUIT:
                NUM_THREADS--;
                bytes_sent += send_formatted_response_to_client(client, 221, "Goodbye.");
                break;
        
            default:
                bytes_sent += send_formatted_response_to_client(client, 502, "Command not implemented.");
                break;
        }
    }
    else {
        switch(command) {
            case USER: 
                bytes_sent += process_user_command(client, parsed[1]);
                break;
            case QUIT:
                NUM_THREADS--;
                bytes_sent += send_formatted_response_to_client(client, 221, "Goodbye.");
                break;
            default:
                bytes_sent += send_formatted_response_to_client(client, 530, "User not logged in.");
                break;
        }
    }
    int i;
    for (i = 0; i < MAX_NUM_ARGS; i++) {
        free(parsed[i]);
    }
    free(parsed);
    
    return bytes_sent;     
} /*    END PROCESS REQUEST    */

char** tokenize_commands(char* buffer) {

// Assuming all commands contain less than two words
    char** parsed = malloc(MAX_NUM_ARGS * sizeof(char *));
    
    // Parse buffer, splitting on spaces, tabs, nl, cr
    int j = 0;
    char* pch;
    pch = strtok(buffer," \t\n\r");
    while (j < MAX_NUM_ARGS) {   
        parsed[j] = malloc(MAX_COMMAND_LENGTH * sizeof(char));
        memset(parsed[j], '\0', MAX_COMMAND_LENGTH);
        snprintf(parsed[j], MAX_COMMAND_LENGTH, "%s", pch);
        printf("PCH: %s\n", pch);
        pch = strtok(NULL, " \t\n\r");
        j++;
    }
    return parsed;
}

int send_formatted_response_to_client(Connection* client, int code, const char* response) {
    char *message = malloc(MAX_MSG_LENGTH);
    memset(message, '\0', MAX_MSG_LENGTH);
    
    snprintf(message, MAX_MSG_LENGTH, "%d %s\r\n", code, response);
    
    int bytes_sent = send(client->main_socket, message, strlen(message), 0);
    
    printf("Data sent: %s\n", message);
    free(message);
    return bytes_sent;
}

int send_data_to_client(Connection* client, unsigned char* data, int data_size) {
    int bytes_sent = 0;
    bytes_sent += send_formatted_response_to_client(client, 150, "Using binary mode for data connection.");
    
    // Send file byte-by-byte over data socket,
    int i;
    for (i = 0; i < (data_size + 1); i++) {
        bytes_sent += send(client->accept_data_socket, (const void *) &data[i], 1, 0);
    }

    // Data has been sent, now close connection
    close(client->accept_data_socket);
    close(client->listening_data_socket);
    client->accept_data_socket = 0;
    client->listening_data_socket = 0;

    bytes_sent += send_formatted_response_to_client(client, 226, "Transfer complete.");
    
    return(bytes_sent);
}
