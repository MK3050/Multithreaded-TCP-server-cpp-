#include <iostream>
#include <vector>
#include <map>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>
#include <algorithm>

bool send_all(int sock, const char* data, int length) {
    int total = 0;
    while (total < length) {
        int sent = send(sock, data + total, length - total, 0);
        if (sent <= 0) return false;
        total += sent;
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

int main() {
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 10);

    std::cout << "Server running...\n";

    std::vector<int> clients;
    std::map<int, std::string> buffers;

    while (true) {
        fd_set read_fds;
        FD_ZERO(&read_fds);

        FD_SET(server_fd, &read_fds);

        int max_fd = server_fd;

        for (int client : clients) {
            FD_SET(client, &read_fds);
            if (client > max_fd) max_fd = client;
        }

        int activity = select(max_fd + 1, &read_fds, NULL, NULL, NULL);

        if (FD_ISSET(server_fd, &read_fds)) {
            int client_fd = accept(server_fd, NULL, NULL);
            clients.push_back(client_fd);
            buffers[client_fd] = "";
            std::cout << "New client connected\n";
        }

        for (int i = 0; i < clients.size(); i++) {
            int client_fd = clients[i];

            if (FD_ISSET(client_fd, &read_fds)) {
                char temp[1024];
                int bytes = recv(client_fd, temp, sizeof(temp), 0);

                if (bytes <= 0) {
                    std::cout << "Client disconnected\n";
                    close(client_fd);
                    buffers.erase(client_fd);
                    clients.erase(clients.begin() + i);
                    i--;
                    continue;
                }

                // append data
                buffers[client_fd].append(temp, bytes);

                std::string& buffer = buffers[client_fd];

                // process messages
                while (true) {
                    if (buffer.size() < 4) break;

                    uint32_t net_len;
                    std::memcpy(&net_len, buffer.data(), 4);
                    uint32_t len = ntohl(net_len);

                    if (buffer.size() < 4 + len) break;

                    std::string message = buffer.substr(4, len);

                    std::cout << "Message: " << message << "\n";

                    // broadcast
                    for (int client : clients) {
                        if (client != client_fd) {
                            send_message(client, message);
                        }
                    }

                    buffer.erase(0, 4 + len);
                }
            }
        }
    }

    close(server_fd);
    return 0;
}