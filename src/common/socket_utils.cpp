#include "socket_utils.hpp"

#include <unistd.h>
#include <sys/socket.h>
#include <fcntl.h>
#include <cstdio>

void setNonBlocking(int fd)
{
    int flags = fcntl(fd, F_GETFL, 0);

    if (flags == -1) {
        perror("fcntl");
        return;
    }

    if (fcntl(fd, F_SETFL, flags | O_NONBLOCK) == -1) {
        perror("fcntl");
    }
}

bool send_all(int sock, const char* data, int length)
{
    int total = 0;

    while (total < length) {
        int sent = send(sock, data + total, length - total, 0);

        if (sent <= 0)
            return false;

        total += sent;
    }

    return true;
}