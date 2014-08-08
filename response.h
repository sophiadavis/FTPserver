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

void* process_control_connection(void *sock);
int process_request(char *buffer, int new_socket, int bytes_received, int *signed_in, int *data_port, int *listening_data_socket, int *accept_data_socket);
int sign_in_thread(char *username);
int pwd(char *cwd, char *response, size_t cwd_size);
unsigned long getFileLength(FILE *fp);

extern int NUM_THREADS; // want to get this out of here
extern int MAIN_PORT;
extern int CURRENT_CONNECTION_PORT;
extern const char *ROOT;

// Parameters for sending/receiving with client
extern int MAX_NUM_ARGS;
extern int MAX_COMMAND_LENGTH;
extern int MAX_MSG_LENGTH;