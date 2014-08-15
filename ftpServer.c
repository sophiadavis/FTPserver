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
    
    int* new_socket;
    while (1) {
        int listen_status = listen(listening_socket, BACKLOG);
        check_status(listen_status, "listen");
    
        new_socket = malloc(sizeof(int));
        *new_socket = open_socket_for_incoming_connection(listening_socket);
        spawn_thread(new_socket, &process_control_connection);
    }

    close(listening_socket);
    return 0;
}

void spawn_thread(int* new_socket, void *on_create_function) {
    pid_t pID;
    pthread_t tid;
    NUM_THREADS++;
    pthread_create(&tid, NULL, on_create_function, new_socket);
    printf("\nA client has connected (socket %d), new thread created. Total threads: %d.\n", *new_socket, NUM_THREADS);
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

int process_user_command(Connection* client, const char* username) {
    int bytes_sent = 0;
    
    client->sign_in_status = sign_in_client(username);
    
    if (client->sign_in_status == 1) {
        bytes_sent += send_formatted_response_to_client(client, 230, "User signed in. Using binary mode to transfer files.");
    }
    else {
        bytes_sent += send_formatted_response_to_client(client, 530, "Sign in failure");
    }
    return bytes_sent;
}

int sign_in_client(const char *username) {
    if (strcmp(username, "anonymous") == 0) {
        return 1;
    }
    else {
        return -1;
    }
}

int process_pwd_command(Connection* client) {
    int bytes_sent = 0;
    
    char *response = malloc(MAX_MSG_LENGTH); 
    char *cwd = malloc(MAX_MSG_LENGTH);
    if (getcwd(cwd, MAX_MSG_LENGTH) != NULL) {
        snprintf(response, MAX_MSG_LENGTH, "\"%s\"", cwd);
    }
    else {
        perror("getcwd() error");
        exit(1);
    }

    bytes_sent += send_formatted_response_to_client(client, 257, response);
    free(cwd);
    free(response);
    return bytes_sent;
}

int process_cwd_command(Connection* client, const char* dir) {
    int bytes_sent = 0;
    
    ///////////////////////////////////////////
    // THREADS SHARE WORKING DIRECTORY, DAMMIT.
    ///////////////////////////////////////////

    int chdir_status = chdir(dir);
    if (chdir_status == 0) {
        bytes_sent += send_formatted_response_to_client(client, 250, "CWD successful.");
    }
    else {
        perror("CWD");
        bytes_sent += send_formatted_response_to_client(client, 550, "CWD error.");
    }
    return bytes_sent;
}

int process_pasv_command(Connection* client) {
    int bytes_sent = 0;
    
    CURRENT_CONNECTION_PORT++;
    client->data_port = CURRENT_CONNECTION_PORT;
    client->listening_data_socket = open_and_bind_socket_on_port(client->data_port);

    // Client expects (a1,a2,a3,a4,p1,p2), where port = p1*256 + p2
    int p1 = client->data_port / 256;
    int p2 = client->data_port % 256;
    printf("opened new port\n");

    int listen_status = listen(client->listening_data_socket, BACKLOG);
    check_status(listen_status, "listen");
    printf("listening\n");

    char *response = malloc(MAX_MSG_LENGTH);
    snprintf(response, MAX_MSG_LENGTH, "%s =127,0,0,1,%i,%i", "Entering Passive Mode", p1, p2);
    bytes_sent += send_formatted_response_to_client(client, 227, response);

    free(response);

    client->accept_data_socket = open_socket_for_incoming_connection(client->listening_data_socket);
    printf("Am I blocking???\n");
    return bytes_sent;
}

int process_nlst_command(Connection* client) {
    int bytes_sent = 0;

    int data_length = 0;
    int response_length = 0;
    DIR *d;
    struct dirent *dir;
    d = opendir(".");
    char dirInfo[MAX_MSG_LENGTH];

    if (d && client->accept_data_socket > 0) {
        while ((dir = readdir(d)) != NULL) {
            data_length = data_length + snprintf(dirInfo + data_length, MAX_MSG_LENGTH - data_length, "%s \r\n", dir->d_name);
        }
        closedir(d);
        
        bytes_sent += send_data_to_client(client, (unsigned char *) dirInfo, strlen(dirInfo));
    }
    else {
        bytes_sent += send_formatted_response_to_client(client, 550, "NLST error.");
        printf("Data socket is %d\n", client->accept_data_socket);
    }
    return bytes_sent;
}

int process_retr_command(Connection* client, const char* file) {
    int bytes_sent = 0;

    FILE *fp;
    fp = fopen(file, "rb");
    if ((fp != NULL) && client->accept_data_socket > 0) {
              
        unsigned long fileLength = getFileLength(fp);
        unsigned char fileBuffer[fileLength + 1];
        
        if (!fileBuffer) {
            fprintf(stderr, "Memory error!");
            fclose(fp);
            bytes_sent += send_formatted_response_to_client(client, 550, "RETR error.");
            return(bytes_sent);
        }
        fread(fileBuffer, sizeof(unsigned char), fileLength, fp);
        fclose(fp);
        
        bytes_sent += send_data_to_client(client, fileBuffer, fileLength);
    }
    else {
        bytes_sent += send_formatted_response_to_client(client, 550, "RETR error.");
    }
    return bytes_sent;
}

unsigned long getFileLength(FILE *fp) {
    fseek(fp, 0, SEEK_END);
    unsigned long length = ftell(fp);
    rewind(fp);
    return length;
}