#include <iostream>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <unistd.h>
#include <netdb.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <vector>
#include <unordered_map>
#include <stdlib.h>

#define QUEUE_SIZE 5
#define USERNAME_SIZE 256
#define MSG_SIZE 256

struct sockaddr_in server_address;
int server_socket_descriptor;
int connection_socket_descriptor;
int reuse_addr_val = 1;
std::vector<int> clientFds;
std::unordered_map<std::string, int> usersFdsMap;

struct client_thread_data {
    int socket_fd;
};

struct msg_data {
    int fd;
    char* msg;
};

void createServerSocket(short serverPort, char* serverAddress) {
    memset(&server_address, 0, sizeof(struct sockaddr));
    server_address.sin_family = AF_INET;
    server_address.sin_port = htons(serverPort);
    inet_pton(AF_INET, serverAddress, &(server_address.sin_addr.s_addr));

    server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket_descriptor < 0) {
        printf("Error creating server socket %s\n", strerror(errno));
        exit(1);
    } else {
        printf("%s%d%s%d\n", "Created socket server at port ", serverPort, ". File descriptor: ", server_socket_descriptor);
    }
}

void reuseServerSocket() {
     int sock_reuse = setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));
     if (sock_reuse < 0) {
         printf("Error reusing server socket %s\n", strerror(errno));
     } else {
         printf("Reused server socket\n");
     }
}

void bindAddrToSocket(char* serverAddress) {
    int bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
    if (bind_result < 0) {
        printf("Error binding address to server socket %s\n", strerror(errno));
        exit(1);
    } else {
        printf("%s%s%s\n", "Address ", serverAddress, " bind to server socket");
    }
}

void listenOnSocket() {
    int listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
    if (listen_result < 0) {
        printf("Error setting listen queue size %s\n", strerror(errno));
        exit(1);
    } else {
        printf("%s%d\n", "Listen queue size set to ", QUEUE_SIZE);
    }
}

msg_data* decodeMsg(char* msg) {
    auto *msg_data1 = new msg_data;
    memset(msg_data1, 0, sizeof(msg_data));
    char username[USERNAME_SIZE], msg_text[MSG_SIZE];
    memset(&username, 0, sizeof(username));
    memset(&msg_text, 0, sizeof(msg_text));
    sscanf(msg, "%[^;];%[^;];\n", username, msg_text);
    msg_text[strlen(msg_text)] = '\n';
    msg_data1->fd = usersFdsMap[username];
    msg_data1->msg = msg_text;
    return msg_data1;
}

void *handleRequest(void *t_data)
{
    pthread_detach(pthread_self());
    auto th_data = (struct client_thread_data*)t_data;
    int client_fd = th_data->socket_fd;
    bool user_registered = false, user_active = true;
    char username[USERNAME_SIZE], msg[MSG_SIZE];
    memset(&username, 0, sizeof(username));
    memset(&msg, 0, sizeof(msg));

    while (user_active) {
        if (!user_registered) {
            read(client_fd, username, sizeof(username));
            username[strlen(username) - 1] = '\0';
            usersFdsMap[username] = client_fd;
            user_registered = true;
        } else {
            read(client_fd, msg, sizeof(msg));
            msg_data* msg_data1 = decodeMsg(msg);
            write(msg_data1->fd, msg_data1->msg, MSG_SIZE);
        }
    }
}

void handleConnection(int client_socket_descriptor) {
    clientFds.push_back(client_socket_descriptor);
    pthread_t thread1;
    auto *client_data = new client_thread_data;
    memset(client_data, 0, sizeof(client_thread_data));
    client_data->socket_fd = client_socket_descriptor;

    int create_result = pthread_create(&thread1, nullptr, handleRequest, (void *)client_data);
    if (create_result){
        printf("Error creating thread for handling requests %s\n", strerror(errno));
        exit(-1);
    }
}

int main(int argc, char* argv[]) {
    createServerSocket(static_cast<short>(std::stoi(argv[1])), argv[2]);
    reuseServerSocket();
    bindAddrToSocket(argv[2]);
    listenOnSocket();

    while(1) {
        connection_socket_descriptor = accept(server_socket_descriptor, nullptr, nullptr);
        if (connection_socket_descriptor < 0) {
            printf("Error connecting new client %s\n", strerror(errno));
            exit(1);
        } else {
            printf("%s%d\n", "New client connected with fd: ", connection_socket_descriptor);
        }

        handleConnection(connection_socket_descriptor);
    }

    return 0;
}