#include "ftpServer.h"

typedef struct connection {
    int sign_in_status;
    int data_port;
    
    int main_socket;
    int listening_data_socket;
    int accept_data_socket;
    
    int total_bytes_sent;
} Connection;

int translate_command(const char* command);

void* process_control_connection(void *sock);

int process_request(char *buffer, Connection* client, int bytes_received);

int send_formatted_response_to_client(Connection* client, int code, const char* message);
int send_data_to_client(Connection* client, unsigned char* data, int data_size);

int process_user_command(Connection* client, const char* username);
int sign_in_client(const char *username);

int process_pwd_command(Connection* client);

int process_cwd_command(Connection* client, const char* dir);

int process_pasv_command(Connection* client);

int process_nlst_command(Connection* client);
int process_retr_command(Connection* client, const char* file);
unsigned long getFileLength(FILE *fp);