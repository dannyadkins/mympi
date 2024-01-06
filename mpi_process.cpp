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
#include <sstream>
// accumulate 
#include <numeric>

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

    // New functionality: disconnect from a process
    void disconnect_from_process(int proc_id) {
        std::cout << "Disconnecting from process " << proc_id << std::endl;
        connection_mutex.lock();
        auto it = connections.find(proc_id);
        if (it != connections.end()) {
            close(it->second);
            connections.erase(it);
            std::cout << "Successfully disconnected from process " << proc_id << std::endl;
        } else {
            std::cout << "No connection found for process " << proc_id << std::endl;
        }
        connection_mutex.unlock();
    }

    ~MPIProcess() {
        std::cout << "Closing listen socket" << std::endl;
        close(listen_sock);
    }
};

int main(int argc, char* argv[]) {
    if (argc < 2) {
        std::cerr << "Usage: " << argv[0] << " [port]" << std::endl;
        return 1;
    }

    int port = std::stoi(argv[1]);
    MPIProcess proc(port);
    std::cout << "MPIProcess created on port " << port << std::endl;

    std::string input, cmd;
    int remote_port;
    std::string message;

    while (true) {
        std::cout << "Enter command: ";
        std::getline(std::cin, input);
        std::istringstream iss(input);

        iss >> cmd;
        if (cmd == "connect") {
            iss >> remote_port;
            proc.connect_to_process(1, "127.0.0.1", remote_port);
        } else if (cmd == "send") {
            iss >> remote_port;
            if (iss.fail()) {
                std::cerr << "Invalid port number." << std::endl;
                continue;
            }
            iss.ignore(std::numeric_limits<std::streamsize>::max(), '\n');
            std::getline(std::cin, message);
            proc.connect_to_process(1, "127.0.0.1", remote_port);
            proc.send_message(1, message);
        } else if (cmd == "disconnect") {
            iss >> remote_port;
            proc.disconnect_from_process(remote_port);
        } else {
            std::cout << "Unknown command" << std::endl;
        }
    }

    return 0;
}

