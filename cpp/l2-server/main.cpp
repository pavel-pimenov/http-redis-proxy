#include <iostream>
#include <cstring>
#include <sys/socket.h>
#include <netinet/in.h>
#include <unistd.h>
#include <thread>
#include <vector>

class EchoServer {
private:
    int server_fd;
    int port;
    bool running;

    void handle_client(int client_fd) {
        char buffer[1024];
        while (true) {
            ssize_t bytes_read = read(client_fd, buffer, sizeof(buffer));
            if (bytes_read <= 0) {
                break;
            }
            write(client_fd, buffer, bytes_read);
        }
        close(client_fd);
    }

public:
    EchoServer(int p) : port(p), running(true) {
        server_fd = socket(AF_INET, SOCK_STREAM, 0);
        if (server_fd < 0) {
            std::cerr << "Failed to create socket" << std::endl;
            exit(1);
        }

        int opt = 1;
        setsockopt(server_fd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt));

        sockaddr_in address;
        address.sin_family = AF_INET;
        address.sin_addr.s_addr = INADDR_ANY;
        address.sin_port = htons(port);

        if (bind(server_fd, (sockaddr*)&address, sizeof(address)) < 0) {
            std::cerr << "Failed to bind to port " << port << std::endl;
            exit(1);
        }

        if (listen(server_fd, 10) < 0) {
            std::cerr << "Failed to listen" << std::endl;
            exit(1);
        }
    }

    ~EchoServer() {
        running = false;
        close(server_fd);
    }

    void run() {
        std::cout << "Echo server running on port " << port << std::endl;
        std::vector<std::thread> threads;

        while (running) {
            sockaddr_in client_addr;
            socklen_t client_len = sizeof(client_addr);
            int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_len);
            if (client_fd < 0) {
                if (running) {
                    std::cerr << "Failed to accept connection" << std::endl;
                }
                continue;
            }

            threads.emplace_back(&EchoServer::handle_client, this, client_fd);
        }

        for (auto& t : threads) {
            if (t.joinable()) {
                t.join();
            }
        }
    }

    void stop() {
        running = false;
        close(server_fd);
    }
};

int main() {
    EchoServer server(3000);
    std::thread server_thread(&EchoServer::run, &server);

    std::cout << "Press Enter to stop..." << std::endl;
    std::cin.get();

    server.stop();
    server_thread.join();

    return 0;
}
