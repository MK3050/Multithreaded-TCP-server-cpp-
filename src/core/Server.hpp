#pragma once

#include <map>
#include <sys/event.h>

#include "Connection.hpp"

class Server {
private:
    int port;
    int server_fd;
    int kq;

    std::map<int, Connection> clients;

    static const int MAX_EVENTS = 32;
    struct kevent events[MAX_EVENTS];

public:
    Server(int port);

    void run();

private:
    void setupSocket();
    void setupKqueue();

    void acceptClient();

    void handleReadEvent(int fd);
    void handleWriteEvent(int fd);

    void disconnectClient(int fd);

    void broadcastMessage(int sender_fd,
                          const std::string& message);
};