#include <iostream>
#include <netinet/in.h>
#include <sys/types.h>

using namespace std;

struct sockaddr_in server_address;
int server_socket_descriptor;

void createServerSocket(short serverPort) {
    memset(&server_address, 0, sizeof(struct sockaddr));
    server_address.sin_family = AF_INET;
    server_address.sin_addr.s_addr = htonl(INADDR_ANY);
    server_address.sin_port = htons(serverPort);

    server_socket_descriptor = socket(AF_INET, SOCK_STREAM, 0);

    if (server_socket_descriptor < 1) {
        fprintf(stderr, "Error creating the socket");
        exit(1);
    }
}

int main(int argc, char* argv[]) {
    createServerSocket(static_cast<short>(stoi(argv[1])));
    return 0;
}