#include <iostream>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>

class Socket {
    int sockfd;
public:
    Socket() {
        sockfd = socket(AF_INET, SOCK_STREAM, 0);
        if (sockfd < 0) {
            perror("socket failed");
            exit(1);
        }
    }

    int get_port() {
        sockaddr_in addr;
        socklen_t len = sizeof(addr);
        if (getsockname(sockfd, (struct sockaddr*)&addr, &len) < 0) {
            perror("getsockname failed");
            return -1;
        }

        return ntohs(addr.sin_port);
    }

    // destructor for cleanup 
    ~Socket() {
        std::cout << "socket destroyed on port " << get_port() << std::endl;
        close(sockfd);
    }

    int get_available_port() {
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
};

int main() {
    Socket socket;
    int port = socket.get_available_port();
    
    if (port < 0) {
        return 1;  // if no port available 
    }

    std::cout << "Server running on port " << port << std::endl;

    return 0;
}
