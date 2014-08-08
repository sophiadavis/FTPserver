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

void check_status(int status, const char *error);
