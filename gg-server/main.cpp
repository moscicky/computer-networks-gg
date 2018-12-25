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

#define QUEUE_SIZE 5

struct sockaddr_in server_address;
int server_socket_descriptor;
int reuse_addr_val = 1;

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

int main(int argc, char* argv[]) {
    createServerSocket(static_cast<short>(std::stoi(argv[1])), argv[2]);
    reuseServerSocket();
    bindAddrToSocket(argv[2]);
    listenOnSocket();
    return 0;
}