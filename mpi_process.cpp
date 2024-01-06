#include <iostream>
#include <string>
#include <map>
#include <vector>
#include <thread>
#include <mutex>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <arpa/inet.h>

class MPIProcess {
    int listen_sock;
    sockaddr_in address;
    std::map<int, int> connections; // Maps process ID to socket FD
    std::mutex connection_mutex;

    void handle_connection(int sock) {
        char buffer[1024] = {0};
        while (true) {
            int bytes_read = read(sock, buffer, 1024);
            if (bytes_read <= 0) {
                std::cout << "Connection closed or error occurred" << std::endl;
                break;
            }
            std::cout << "Received: " << buffer << std::endl;
        }
        std::cout << "Closing socket: " << sock << std::endl;
        close(sock);
    }

public:
    MPIProcess(int port) {
        std::cout << "Initializing MPIProcess on port " << port << std::endl;
        listen_sock = socket(AF_INET, SOCK_STREAM, 0);
        if (listen_sock < 0) {
            perror("socket failed");
            exit(EXIT_FAILURE);
        }

        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(listen_sock, (struct sockaddr *)&address, sizeof(address)) < 0) {
            perror("bind failed");
            exit(EXIT_FAILURE);
        }

        if (listen(listen_sock, 10) < 0) {
            perror("listen");
            exit(EXIT_FAILURE);
        }
        std::cout << "Listening on port " << port << std::endl;

        std::thread([this]() {
            std::cout << "Starting connection handler thread" << std::endl;
            while (true) {
                int new_sock = accept(listen_sock, nullptr, nullptr);
                if (new_sock < 0) {
                    std::cout << "Failed to accept connection" << std::endl;
                    continue;
                }
                std::cout << "Accepted new connection: " << new_sock << std::endl;
                std::thread(&MPIProcess::handle_connection, this, new_sock).detach();
            }
        }).detach();
    }

    void connect_to_process(int proc_id, const std::string& ip, int port) {
        std::cout << "Connecting to process " << proc_id << " at " << ip << ":" << port << std::endl;
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("socket failed");
            return;
        }

        sockaddr_in serv_addr;
        serv_addr.sin_family = AF_INET;
        serv_addr.sin_port = htons(port);

        if (inet_pton(AF_INET, ip.c_str(), &serv_addr.sin_addr) <= 0) {
            perror("Invalid address");
            return;
        }

        if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
            perror("Connection failed");
            return;
        }
        std::cout << "Successfully connected to process " << proc_id << std::endl;

        connection_mutex.lock();
        connections[proc_id] = sock;
        connection_mutex.unlock();
    }

    void send_message(int proc_id, const std::string& message) {
        std::cout << "Sending message to process " << proc_id << ": " << message << std::endl;
        connection_mutex.lock();
        auto it = connections.find(proc_id);
        if (it != connections.end()) {
            send(it->second, message.c_str(), message.size(), 0);
        }
        connection_mutex.unlock();
    }
};

int main(int argc, char* argv[]) {
    if (argc < 3) {
        std::cerr << "Usage: " << argv[0] << " [port] [remote_port]" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);
    int remote_port = std::stoi(argv[2]);

    MPIProcess proc(port);
    std::cout << "MPIProcess created on port " << port << std::endl;

    // Sleep to allow other processes to start
    std::this_thread::sleep_for(std::chrono::seconds(5));

    proc.connect_to_process(1, "127.0.0.1", remote_port);
    proc.send_message(1, "Hello from process " + std::to_string(port));

    // Keep the main thread alive
    while (true) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    return 0;
}
