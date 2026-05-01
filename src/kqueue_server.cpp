#include <iostream>
#include <vector>
#include <map>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <sys/event.h>
#include <cstring>
#include <algorithm>

// ---------- Helpers ----------

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

// ---------- Main ----------

int main() {
    // 1. Create server socket
    int server_fd = socket(AF_INET, SOCK_STREAM, 0);

    sockaddr_in server_addr{};
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(8080);

    bind(server_fd, (sockaddr*)&server_addr, sizeof(server_addr));
    listen(server_fd, 10);

    std::cout << "kqueue server running on port 8080...\n";

    // 2. Create kqueue
    int kq = kqueue();
    if (kq == -1) {
        perror("kqueue");
        return 1;
    }

    // 3. Register server socket
    struct kevent ev;
    EV_SET(&ev, server_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
    kevent(kq, &ev, 1, NULL, 0, NULL);

    // 4. Client tracking
    std::vector<int> clients;
    std::map<int, std::string> buffers;

    const int MAX_EVENTS = 32;
    struct kevent events[MAX_EVENTS];

    // ---------- Event Loop ----------
    while (true) {
        int nev = kevent(kq, NULL, 0, events, MAX_EVENTS, NULL);

        for (int i = 0; i < nev; i++) {
            int fd = events[i].ident;

            // 🔹 New connection
            if (fd == server_fd) {
                int client_fd = accept(server_fd, NULL, NULL);
                if (client_fd < 0) continue;

                clients.push_back(client_fd);
                buffers[client_fd] = "";

                std::cout << "New client connected\n";

                struct kevent client_event;
                EV_SET(&client_event, client_fd, EVFILT_READ, EV_ADD, 0, 0, NULL);
                kevent(kq, &client_event, 1, NULL, 0, NULL);
            }

            // 🔹 Client message
            else {
                char temp[1024];
                int bytes = recv(fd, temp, sizeof(temp), 0);

                if (bytes <= 0) {
                    std::cout << "Client disconnected\n";

                    close(fd);
                    buffers.erase(fd);
                    clients.erase(std::remove(clients.begin(), clients.end(), fd), clients.end());

                    // remove from kqueue
                    struct kevent del_event;
                    EV_SET(&del_event, fd, EVFILT_READ, EV_DELETE, 0, 0, NULL);
                    kevent(kq, &del_event, 1, NULL, 0, NULL);

                    continue;
                }

                // Append incoming data
                buffers[fd].append(temp, bytes);
                std::string& buffer = buffers[fd];

                // Process complete messages
                while (true) {
                    if (buffer.size() < 4) break;

                    uint32_t net_len;
                    std::memcpy(&net_len, buffer.data(), 4);
                    uint32_t len = ntohl(net_len);

                    if (buffer.size() < 4 + len) break;

                    std::string message = buffer.substr(4, len);

                    std::cout << "Message: " << message << "\n";

                    // Broadcast
                    for (int client : clients) {
                        if (client != fd) {
                            if (!send_message(client, message)) {
                                close(client);
                            }
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