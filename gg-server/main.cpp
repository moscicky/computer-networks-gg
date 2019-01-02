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
#include <algorithm>
#include <signal.h>

#define QUEUE_SIZE 5
#define USERNAME_SIZE 256
#define MSG_SIZE 256
#define NEW_USER_CODE 200
#define USER_LEFT_CODE 300
#define NEW_MSG_CODE 400
#define USERS_LIST_CODE 500
#define SERVER_TERMINATED 600

bool serverAlive;

struct sockaddr_in server_address;
int server_socket_descriptor;
int connection_socket_descriptor;
int reuse_addr_val = 1;
struct addrinfo hints, *res, *p;
char ipstr[30];
std::vector<int> clientFds;
std::unordered_map<std::string, int> usersFdsMap;
std::unordered_map<int, pthread_mutex_t> userMutexMap;

pthread_mutex_t clientFdsMutex;
pthread_mutex_t usersFdsMapMutex;

struct client_thread_data {
    int socket_fd;
};


struct msg_data {
    int fd;
    char sender[256];
    char msg[256];
};


/**
 * Set server address and port
 * @param name
 * @param port
 */
void setServerAddress(int argc, char* argv[]) {
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_INET;
    hints.ai_socktype = SOCK_STREAM;
    int get_addr;

    if(argc == 3) {
        get_addr = getaddrinfo(argv[1], argv[2], &hints, &res);
    } else {
        get_addr = getaddrinfo(argv[1], nullptr, &hints, &res);
    }

    if (get_addr != 0) {
        fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(get_addr));
        exit(1);
    }

    for(p = res;p != nullptr; p = p->ai_next) {
        server_address = *(struct sockaddr_in *)p->ai_addr;
        inet_ntop(p->ai_family, &(server_address.sin_addr), ipstr, sizeof ipstr);
        printf("Requested server IP: %s\n", ipstr);
    }

    freeaddrinfo(res);
}

/**
 * Creates server socket
 * @param serverPort
 * @param serverAddress
 */
void createServerSocket() {
    server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket_descriptor < 0) {
        printf("Error creating server socket %s\n", strerror(errno));
        exit(1);
    } else {
        printf("%s%d\n", "Created server socket. File descriptor: ", server_socket_descriptor);
    }
}

/**
 * Helps with reusing sockets of previous server instances
 */
void reuseServerSocket() {
     int sock_reuse = setsockopt(server_socket_descriptor, SOL_SOCKET, SO_REUSEADDR, (char*)&reuse_addr_val, sizeof(reuse_addr_val));
     if (sock_reuse < 0) {
         printf("Error reusing server socket %s\n", strerror(errno));
     } else {
         printf("Reused server socket\n");
     }
}

/**
 * Binds given address to socket
 * @param serverAddress
 */
void bindAddrToSocket(char* serverAddress) {
    int bind_result = bind(server_socket_descriptor, (struct sockaddr*)&server_address, sizeof(struct sockaddr));
    if (bind_result < 0) {
        printf("Error binding address to server socket %s\n", strerror(errno));
        exit(1);
    } else {
        printf("%s%s%s%s%s\n", "Address ", ipstr, ":", serverAddress, " bind to server socket");
    }
}

/**
 * Makes server listen on socket
 */
void listenOnSocket() {
    int listen_result = listen(server_socket_descriptor, QUEUE_SIZE);
    if (listen_result < 0) {
        printf("Error setting listen queue size %s\n", strerror(errno));
        exit(1);
    } else {
        printf("%s%d\n", "Listen queue size set to ", QUEUE_SIZE);
    }
}

/**
 * Decodes raw message
 * @param msg
 * @return
 */
msg_data* decodeMsg(char* msg) {
    auto *msg_data1 = new msg_data;
    memset(msg_data1, 0, sizeof(msg_data));
    char username[USERNAME_SIZE], msg_text[MSG_SIZE];
    memset(&username, 0, sizeof(username));
    memset(&msg_text, 0, sizeof(msg_text));
    sscanf(msg, "%[^;];%[^;];\n", username, msg_text);
    msg_text[strlen(msg_text)] = '\n';

    msg_data1->fd = usersFdsMap[username];
    memset(msg_data1->msg, 0, sizeof(msg_data1->msg));
    strcpy(msg_data1->sender, username);
    strcpy(msg_data1->msg, msg_text);
    return msg_data1;
}

/**
 * Broadcasts new user other users
 * @param username
 * @param fd
 */
void broadcastNewUser(char *username, int fd) {
    std::string name(username);
    std::string new_user_msg = std::to_string(NEW_USER_CODE) + ";" + name + ";";
    std::string users_list = std::to_string(USERS_LIST_CODE) + ";";


    pthread_mutex_lock(&clientFdsMutex);
    for (int i = 0; i < clientFds.size(); i++) {
        auto index = static_cast<unsigned long>(i);

        if (clientFds.at(index) != fd) {
            pthread_mutex_lock(&userMutexMap[clientFds.at(index)]);
            write(clientFds.at(index), new_user_msg.c_str(), new_user_msg.size());
            pthread_mutex_unlock(&userMutexMap[clientFds.at(index)]);
        }
    }
    pthread_mutex_unlock(&clientFdsMutex);

    pthread_mutex_lock(&usersFdsMapMutex);
    for (auto& it: usersFdsMap) {
        users_list += (it.first + ";");
    }
    pthread_mutex_unlock(&usersFdsMapMutex);

    pthread_mutex_lock(&userMutexMap[fd]);
    write(fd, users_list.c_str(), users_list.size());
    pthread_mutex_unlock(&userMutexMap[fd]);
}

/**
 * Broadcasts info about user who left
 * @param username of the user who left
 */
void broadcastUserLeft(char *username, int client_fd) {
    std::string name(username);
    std::string new_user_msg = std::to_string(USER_LEFT_CODE) + ";" + name + ";";

    pthread_mutex_lock(&clientFdsMutex);
    clientFds.erase(std::remove(clientFds.begin(), clientFds.end(), client_fd), clientFds.end());

    for (int i = 0; i < clientFds.size(); i++) {
        auto index = static_cast<unsigned long>(i);
        pthread_mutex_lock(&userMutexMap[clientFds.at(index)]);
        write(clientFds.at(index), new_user_msg.c_str(), new_user_msg.size());
        pthread_mutex_unlock(&userMutexMap[clientFds.at(index)]);
    }
    pthread_mutex_unlock(&clientFdsMutex);

    pthread_mutex_lock(&usersFdsMapMutex);
    usersFdsMap.erase(name);
    pthread_mutex_unlock(&usersFdsMapMutex);
}

/**
 * Prepares msg to the form understandable by the server
 * @param sender
 * @param msg
 * @return
 */
std::string prepareMsg(char *sender, char *msg) {
    msg[strlen(msg) - 1] = '\0';
    std::string sender_name(sender);
    std::string msg_text(msg);
    std::string new_msg = std::to_string(NEW_MSG_CODE) + ";" + sender_name + ";" + msg_text + ";\n";

    return new_msg;
}

/**
 * Broadcasts new user to other users, initializes it's mutex/
 * @param client_fd new user socket file descriptor id
 * @param username name of the new user
 */
void handleNewUser(int client_fd, char* username) {
    userMutexMap[client_fd] = PTHREAD_MUTEX_INITIALIZER;
    broadcastNewUser(username, client_fd);

    pthread_mutex_lock(&usersFdsMapMutex);
    usersFdsMap[username] = client_fd;
    pthread_mutex_unlock(&usersFdsMapMutex);
}

/**
 * Decodes raw msg and sends it to decoded client
 * @param raw_msg
 * @param sender
 */
void handleNewMsg(char* raw_msg, char* sender) {
    msg_data* msg_info = decodeMsg(raw_msg);
    std::string message = prepareMsg(sender, msg_info->msg);

    pthread_mutex_lock(&userMutexMap[msg_info->fd]);
    write(msg_info->fd, message.c_str(), message.size());
    pthread_mutex_unlock(&userMutexMap[msg_info->fd]);
}

/**
 * Handles one client requests - decodes it's messages and sends them to other clients
 * @param t_data client's file descriptor
 * @return
 */
void *handleRequest(void *t_data) {
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
            handleNewUser(client_fd, username);
            user_registered = true;
        } else {
            int read_result = static_cast<int>(read(client_fd, msg, sizeof(msg)));

            if (read_result < 1) {
                user_active = false;
                broadcastUserLeft(username, client_fd);
            } else {
                handleNewMsg(msg, username);
            }
        }
    }

    pthread_exit(nullptr);
    return nullptr;
}

/**
 * Adds new client's socket fd to the vector of clients' fds and created thread responsible for handling client requests
 * @param client_socket_descriptor
 */
void handleConnection(int client_socket_descriptor) {
    pthread_mutex_lock(&clientFdsMutex);
    clientFds.push_back(client_socket_descriptor);
    pthread_mutex_unlock(&clientFdsMutex);

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

void terminationHandler(int boilerplate) {
    std::string term_msg = std::to_string(SERVER_TERMINATED) + ";";

    pthread_mutex_lock(&clientFdsMutex);

    for (int i = 0; i < clientFds.size(); i++) {
        auto index = static_cast<unsigned long>(i);
        pthread_mutex_lock(&userMutexMap[clientFds.at(index)]);
        write(clientFds.at(index), term_msg.c_str(), term_msg.size());
        pthread_mutex_unlock(&userMutexMap[clientFds.at(index)]);
    }

    pthread_mutex_unlock(&clientFdsMutex);
    serverAlive = false;
    exit(1);
}

void initLocks() {
    clientFdsMutex = PTHREAD_MUTEX_INITIALIZER;
    usersFdsMapMutex = PTHREAD_MUTEX_INITIALIZER;
}

int main(int argc, char* argv[]) {
    serverAlive = true;
    initLocks();
    setServerAddress(argc, argv);
    createServerSocket();
    reuseServerSocket();
    bindAddrToSocket(argv[2]);
    listenOnSocket();
    signal(SIGINT, terminationHandler);

    while(serverAlive) {
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