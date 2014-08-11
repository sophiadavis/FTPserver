/* ftpServer.c
*  Coordinates high-level creation of sockets, listening for incoming connections,
*  and spawning new threads to deal with each client.
*/

#include "ftpServer.h"
#include "response.h"
#include "socketConnection.h"
       
int NUM_THREADS = 0;
int BACKLOG = 10;
int MAIN_PORT = 5000;
int CURRENT_CONNECTION_PORT = 5000;
const char *ROOT = "/var/folders/r6/mzb0s9jd1639123lkcsv4mf00000gn/T/server";

// Parameters for sending/receiving with client
int MAX_NUM_ARGS = 2;
int MAX_COMMAND_LENGTH = 50;
int MAX_MSG_LENGTH = 1024 * sizeof(char);

int main(int argc, char *argv[]){

    set_root(ROOT);
    
    int listening_socket = open_and_bind_socket_on_port(MAIN_PORT);
    
    int new_socket;
    while (1) {
        int listen_status = listen(listening_socket, BACKLOG);
        check_status(listen_status, "listen");
    
        new_socket = open_socket_for_incoming_connection(listening_socket);
        spawn_thread(new_socket, &process_control_connection);
    }

    close(listening_socket);
    return 0;
}

void spawn_thread(int new_socket, void *on_create_function) {
    pid_t pID;
    pthread_t tid;
    NUM_THREADS++;
    pthread_create(&tid, NULL, on_create_function, &new_socket);
    printf("\nA client has connected (socket %d), new thread created. Total threads: %d.\n", new_socket, NUM_THREADS);
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

// Set chroot jail for server
void set_root(const char* path) {
    int chdir_status = chdir(path);
    check_status(chdir_status, "chdir");
//     int chroot_status = chroot(path);
//     check_status(chroot_status, "chroot");
}
