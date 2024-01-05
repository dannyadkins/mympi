#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

int get_available_port(int& sockfd) {
    sockaddr_in addr;
    addr.sin_family = AF_INET;
    addr.sin_addr.s_addr = htonl(INADDR_ANY);
    addr.sin_port = 0;  // this allows the OS to choose whatever port is available

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
        return 1;  // if no port available 
    }

    std::cout << "Server running on port " << port << std::endl;

    close(sockfd);
    return 0;
}

