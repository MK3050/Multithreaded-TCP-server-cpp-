#pragma once

#include <string>

class Connection {
public:
    int fd;

    std::string username;

    std::string inputBuffer;
    std::string outputBuffer;

    Connection();
    Connection(int client_fd);
};