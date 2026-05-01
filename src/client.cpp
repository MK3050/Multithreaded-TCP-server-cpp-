#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <cstring>
#include <thread>


bool send_all(int sock, const char *data, int length)
{
    int total = 0;
    while (total < length)
    {
        int sent = send(sock, data + total, length - total, 0);
        if (sent <= 0)
            return false;
        total += sent;
    }
    return true;
}

bool recv_all(int sock, char *buffer, int length)
{
    int total = 0;
    while (total < length)
    {
        int bytes = recv(sock, buffer + total, length - total, 0);
        if (bytes <= 0)
            return false;
        total += bytes;
    }
    return true;
}
void receive_messages(int sock) {
    while (true) {
        uint32_t net_len;

        // read length
        if (!recv_all(sock, (char*)&net_len, sizeof(net_len))) {
            std::cout << "\nDisconnected from server\n";
            break;
        }

        uint32_t len = ntohl(net_len);

        std::vector<char> buffer(len);

        // read message
        if (!recv_all(sock, buffer.data(), len)) {
            std::cout << "\nDisconnected from server\n";
            break;
        }

        std::string msg(buffer.begin(), buffer.end());

        std::cout << "\nServer: " << msg << "\n> ";
        std::cout.flush();
    }
}
int main()
{
    // 1. Create socket
    int sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock == -1)
    {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    // 2. Define server address
    sockaddr_in server_addr;
    std::memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);

    // Convert IP string to binary
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    // 3. Connect to server
    if (connect(sock, (sockaddr *)&server_addr, sizeof(server_addr)) < 0)
    {
        std::cerr << "Connection failed\n";
        return 1;
    }

    std::cout << "Connected to server!\n";
    std::thread recv_thread(receive_messages, sock);
    while (true)
    {
        // 4. Send message
        std::string msg;
        std::cout << "Enter message to send: ";
        std::getline(std::cin, msg);
        uint32_t len = msg.size();
        uint32_t net_len = htonl(len);

        send_all(sock, (char *)&net_len, sizeof(net_len));
        send_all(sock, msg.c_str(), msg.size());
    }
    recv_thread.join();

    // 6. Close
    close(sock);

    return 0;
}