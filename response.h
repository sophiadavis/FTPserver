#include <stdio.h>    
#include <stdlib.h>
#include <string.h> 
#include <netinet/in.h>   
#include <sys/socket.h>    
#include <sys/stat.h>    
#include <sys/types.h>    
#include <unistd.h>  
#include <netdb.h>
#include <dirent.h>
#include <pthread.h>

typedef struct connection {
    int sign_in_status;
    int data_port;
    
    int main_socket;
    int listening_data_socket; // what are these???
    int accept_data_socket;
    
    int total_bytes_sent;
} Connection;

void* process_control_connection(void *sock);
int send_formatted_response_to_socket(int new_socket, int code, const char* message);
int process_request(char *buffer, Connection* client, int bytes_received);
int sign_in_client(const char *username);
int pwd(char *cwd, char *response, size_t cwd_size);
unsigned long getFileLength(FILE *fp);
int translate_command(const char* command);

int process_user_command(Connection* client, const char* username);
int process_pwd_command(Connection* client);
int process_cwd_command(Connection* client, const char* dir);
int process_pasv_command(Connection* client);
int process_nlst_command(Connection* client);
int process_retr_command(Connection* client, const char* file);

extern int NUM_THREADS; // want to get this out of here
extern int MAIN_PORT;
extern int BACKLOG;
extern int CURRENT_CONNECTION_PORT;
extern const char *ROOT;

// Parameters for sending/receiving with client
extern int MAX_NUM_ARGS;
extern int MAX_COMMAND_LENGTH;
extern int MAX_MSG_LENGTH;