#include "Connection.hpp"

Connection::Connection()
{
    fd = -1;
}

Connection::Connection(int client_fd)
{
    username = "Anonymous";
    fd = client_fd;
}