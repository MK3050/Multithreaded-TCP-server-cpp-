#include <iostream>
#include <unistd.h>
#include <arpa/inet.h>
#include <thread>
#include <vector>

#include "../common/socket_utils.hpp"
#include "../common/protocol.hpp"

bool recv_all(int sock, char* buffer, int length)
{
    int total = 0;

    while (total < length) {

        int bytes = recv(sock,
                         buffer + total,
                         length - total,
                         0);

        if (bytes <= 0)
            return false;

        total += bytes;
    }

    return true;
}

// =====================================================
// RECEIVE THREAD
// =====================================================

void receive_messages(int sock)
{
    while (true) {

        uint32_t net_len;

        // ---------- Read Length ----------
        if (!recv_all(sock,
                      (char*)&net_len,
                      sizeof(net_len))) {

            std::cout << "\nDisconnected from server\n";
            break;
        }

        uint32_t len = ntohl(net_len);

        std::vector<char> buffer(len);

        // ---------- Read Message ----------
        if (!recv_all(sock,
                      buffer.data(),
                      len)) {

            std::cout << "\nDisconnected from server\n";
            break;
        }

        std::string msg(buffer.begin(),
                        buffer.end());

        std::cout << "\nServer: "
                  << msg
                  << "\n> ";

        std::cout.flush();
    }
}

// =====================================================
// MAIN
// =====================================================

int main()
{
    // ---------- Create Socket ----------
    int sock = socket(AF_INET,
                      SOCK_STREAM,
                      0);

    if (sock == -1) {
        std::cerr << "Socket creation failed\n";
        return 1;
    }

    // ---------- Configure Server Address ----------
    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(8080);

    inet_pton(AF_INET,
              "127.0.0.1",
              &server_addr.sin_addr);

    // ---------- Connect ----------
    if (connect(sock,
                (sockaddr*)&server_addr,
                sizeof(server_addr)) < 0) {

        std::cerr << "Connection failed\n";
        return 1;
    }

    std::cout << "Connected to server!\n";

    // ---------- Start Receive Thread ----------
    std::thread recv_thread(receive_messages,
                            sock);

    // =================================================
    // SEND LOOP
    // =================================================

    while (true) {

        std::string msg;

        std::cout << "> ";

        std::getline(std::cin, msg);

        // ---------- Serialize Message ----------
        std::string output;

        Protocol::appendMessage(output,
                                msg);

        // ---------- Send ----------
        if (!send_all(sock,
                      output.data(),
                      output.size())) {

            std::cout << "Send failed\n";
            break;
        }
    }

    recv_thread.join();

    close(sock);

    return 0;
}