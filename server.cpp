#include <iostream>
#include <unistd.h>      // close()
#include <sys/socket.h>  // socket()
#include <netinet/in.h>  // sockaddr_in
#include <cstring>       // memset
#include <thread>
#include <vector>
#include <mutex>
using namespace std;

std::vector<int> clients;
std::mutex clients_mutex;

bool send_all(int sock, const char* data, int length) {
    int total = 0;
    while (total < length) {
        int sent = send(sock, data + total, length - total, 0);
        if (sent <= 0) return false;
        total += sent;
    }
    return true;
}

bool recv_all(int sock, char* buffer, int length) {
    int total = 0;
    while (total < length) {
        int bytes = recv(sock, buffer + total, length - total, 0);
        if (bytes <= 0) return false;
        total += bytes;
    }
    return true;
}
bool send_message(int sock, const std::string& msg) {
    uint32_t len = msg.size();
    uint32_t net_len = htonl(len);

    if (!send_all(sock, (char*)&net_len, sizeof(net_len))) return false;
    if (!send_all(sock, msg.c_str(), msg.size())) return false;

    return true;
}
void broadcast(int sender_fd, const std::string& message) {
    std::vector<int> snapshot;

    // copy clients safely
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        snapshot = clients;
    }

    // track dead clients
    std::vector<int> dead_clients;

    for (int client : snapshot) {
        if (!send_message(client, message)) {
            dead_clients.push_back(client);
        }
    }

    // remove dead clients
    if (!dead_clients.empty()) {
        std::lock_guard<std::mutex> lock(clients_mutex);

        for (int dead : dead_clients) {
            clients.erase(std::remove(clients.begin(), clients.end(), dead), clients.end());
            close(dead);
            std::cout << "Removed dead client\n";
        }
    }
}

void handle_client(int client_fd) {
    while (true) {
        uint32_t net_len;

        // 1. read length
        if (!recv_all(client_fd, (char*)&net_len, sizeof(net_len))) {
            std::cout << "Client disconnected\n";
            break;
        }

        uint32_t len = ntohl(net_len);

        std::vector<char> buffer(len);

        // 2. read message
        if (!recv_all(client_fd, buffer.data(), len)) {
            std::cout << "Client disconnected\n";
            break;
        }

        std::string message(buffer.begin(), buffer.end());

        std::cout << "Message: " << message << std::endl;

        broadcast(client_fd, message);
    }

    // remove client safely
    {
        std::lock_guard<std::mutex> lock(clients_mutex);
        clients.erase(std::remove(clients.begin(), clients.end(), client_fd), clients.end());
    }

    close(client_fd);
}
int main() {
    // 1. Create socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);
    if (server_fd == -1) {
        cerr << "Socket creation failed\n";
        return 1;
    }

    // 2. Define server address
    sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY; // listen on all interfaces
    server_addr.sin_port = htons(8080);       // port 8080

    
    // 3. Bind socket
    if (::bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        cerr << "Bind failed\n";
        return 1;
    }

    // 4. Listen
    if (listen(server_fd, 5) < 0) {
        cerr << "Listen failed\n";
        return 1;
    }

    cout << "Server listening on port 8080...\n";

    // 5. Accept connection
    sockaddr_in client_addr;
    socklen_t client_size = sizeof(client_addr);

    while (true) {
    int client_fd = accept(server_fd, (sockaddr*)&client_addr, &client_size);

    if (client_fd < 0) {
        std::cerr << "Accept failed\n";
        continue;
    }

    std::cout << "New client connected\n";

    std::lock_guard<std::mutex> lock(clients_mutex);
    clients.push_back(client_fd);

    std::thread t(handle_client, client_fd);
    t.detach();  // run independently
}

    cout << "Client connected!\n";

    
    
    // 7. Close sockets
    close(server_fd);

    return 0;
}