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

extern int NUM_THREADS; // want to get this out of here
extern int MAIN_PORT;
extern int BACKLOG;
extern int CURRENT_CONNECTION_PORT;
extern const char *ROOT;

// Parameters for sending/receiving with client
extern int MAX_NUM_ARGS;
extern int MAX_COMMAND_LENGTH;
extern int MAX_MSG_LENGTH;

void set_root(const char* path);
void check_status(int status, const char *error);
