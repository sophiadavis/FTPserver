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
#include <limits.h>

extern int NUM_THREADS;
extern int MAIN_PORT;
extern int BACKLOG;
extern int CURRENT_CONNECTION_PORT;
extern const char *ROOT;

// Parameters for sending/receiving with client
extern int MAX_NUM_ARGS;
extern int MAX_COMMAND_LENGTH;
extern int MAX_MSG_LENGTH;

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

void spawn_thread(int* new_socket, void *on_create_function);
void check_status(int status, const char *error);
void set_root(const char* path);

int process_user_command(Connection* client, const char* username);
int sign_in_client(const char *username);

int process_pwd_command(Connection* client);
int process_cwd_command(Connection* client, const char* dir);
int process_pasv_command(Connection* client);

int process_nlst_command(Connection* client);
int process_retr_command(Connection* client, const char* file);
unsigned long getFileLength(FILE *fp);
