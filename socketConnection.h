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

int open_and_bind_socket_on_port(int port);
struct sockaddr_in use_ipv4_address(int port);
struct addrinfo set_socket_address_information(int port, struct sockaddr_in address_in);
int create_socket_with_port_and_address(int port, struct addrinfo address);
void set_reuse_port_option(int listening_socket);
int bind_with_error_checking(int listening_socket, struct sockaddr_in address_in);
int begin_connection(int listening_socket, void *on_create_function);

extern int NUM_THREADS; // want to get this out of here
extern int MAIN_PORT;
extern int CURRENT_CONNECTION_PORT;
extern const char *ROOT;

// Parameters for sending/receiving with client
extern int MAX_NUM_ARGS; // want to get this out of here
extern int MAX_COMMAND_LENGTH; // want to get this out of here
extern int MAX_MSG_LENGTH; // want to get this out of here