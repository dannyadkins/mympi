#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int get_available_port(int& sockfd) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = 0;  // Let OS choose the port

    if (bind(sockfd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        perror("bind failed");
        return -1;
    }

    socklen_t len = sizeof(addr);
    if (getsockname(sockfd, (struct sockaddr*)&addr, &len) < 0) {
        perror("getsockname failed");
        return -1;
    }

    return ntohs(addr.sin_port);
}

int main() {
    int sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd < 0) {
        perror("socket failed");
        return 1;
    }

    int port = get_available_port(sockfd);
    if (port < 0) {
        return 1;  // Port assignment failed
    }

    std::cout << "Server running on port " << port << std::endl;

    // Additional code for listening and accepting connections

    close(sockfd);
    return 0;
}

