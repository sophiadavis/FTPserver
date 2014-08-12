#include "ftpServer.h"

#ifndef CONNECTION
#define CONNECTION
    typedef struct connection {
        int sign_in_status;
        int data_port;
    
        int main_socket;
        int listening_data_socket;
        int accept_data_socket;
    
        int total_bytes_sent;
    } Connection;
#endif

int translate_command(const char* command);

void* process_control_connection(void *sock);
int process_request(char *buffer, Connection* client, int bytes_received);
char** tokenize_commands(char* buffer);

int send_formatted_response_to_client(Connection* client, int code, const char* message);
int send_data_to_client(Connection* client, unsigned char* data, int data_size);
