#include "ftpServer.h"
#include "response.h"
#include "socketConnection.h"
       
int NUM_THREADS = 0;
int MAIN_PORT = 5000;
int CURRENT_CONNECTION_PORT = 5000;
const char *ROOT = "/var/folders/r6/mzb0s9jd1639123lkcsv4mf00000gn/T/server";

// Parameters for sending/receiving with client
int MAX_NUM_ARGS = 2;
int MAX_COMMAND_LENGTH = 50;
int MAX_MSG_LENGTH = 1024 * sizeof(char);

int main(int argc, char *argv[]){

    set_root(ROOT);
    
// For storing results from creating socket
//     struct addrinfo *results;
    
    // Create and bind socket to specified port
//     int listening_socket = prepare_socket(MAIN_PORT, results);
    int listening_socket = open_and_bind_socket_on_port(MAIN_PORT);
    
    // Listen for connections
    pid_t pID;
    int new_socket;
    while (1) {
    
        int listen_status = listen(listening_socket, 10);
        check_status(listen_status, "listen");
    
        // Accept clients, spawn new thread for each connection
        new_socket = begin_connection(listening_socket, &process_control_connection);
    }
//     freeaddrinfo(results);
    close(new_socket);
    close(listening_socket);
    return 0;
}

// Sets appropriate error message if status indicates error
void check_status(int status, const char *error) {
    if (status < 0) {
        char message[100];
        sprintf(message, "server: %s", error);
        perror(message);
        exit(1);
    }
}

void set_root(const char* path) {
    int chdir_status = chdir(path);
    check_status(chdir_status, "chdir");
//     int chroot_status = chroot(path);
//     check_status(chroot_status, "chroot");
}
