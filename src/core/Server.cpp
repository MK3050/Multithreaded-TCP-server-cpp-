#include "Server.hpp"

#include <iostream>
#include <unistd.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <cstring>

#include "../common/socket_utils.hpp"
#include "../common/protocol.hpp"

// =====================================================
// CONSTRUCTOR
// =====================================================

Server::Server(int port)
{
    this->port = port;

    setupSocket();
    setupKqueue();
}

// =====================================================
// SOCKET SETUP
// =====================================================

void Server::setupSocket()
{
    server_fd = socket(AF_INET, SOCK_STREAM, 0);

    if (server_fd < 0)
    {
        perror("socket");
        exit(1);
    }

    setNonBlocking(server_fd);

    sockaddr_in server_addr{};

    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(port);

    if (bind(server_fd,
             (sockaddr *)&server_addr,
             sizeof(server_addr)) < 0)
    {

        perror("bind");
        exit(1);
    }

    if (listen(server_fd, 10) < 0)
    {
        perror("listen");
        exit(1);
    }

    std::cout << "kqueue server running on port "
              << port
              << "...\n";
}

// =====================================================
// KQUEUE SETUP
// =====================================================

void Server::setupKqueue()
{
    kq = kqueue();

    if (kq == -1)
    {
        perror("kqueue");
        exit(1);
    }

    struct kevent ev;

    EV_SET(&ev,
           server_fd,
           EVFILT_READ,
           EV_ADD,
           0,
           0,
           NULL);

    kevent(kq,
           &ev,
           1,
           NULL,
           0,
           NULL);
}

// =====================================================
// MAIN EVENT LOOP
// =====================================================

void Server::run()
{
    while (true)
    {

        int nev = kevent(kq,
                         NULL,
                         0,
                         events,
                         MAX_EVENTS,
                         NULL);

        if (nev < 0)
        {
            perror("kevent");
            continue;
        }

        for (int i = 0; i < nev; i++)
        {

            int fd = events[i].ident;

            // ---------- New Connection ----------
            if (fd == server_fd)
            {
                acceptClient();
            }

            // ---------- Writable Socket ----------
            else if (events[i].filter == EVFILT_WRITE)
            {
                handleWriteEvent(fd);
            }

            // ---------- Readable Socket ----------
            else
            {
                handleReadEvent(fd);
            }
        }
    }
}

// =====================================================
// ACCEPT CLIENT
// =====================================================

void Server::acceptClient()
{
    int client_fd = accept(server_fd,
                           NULL,
                           NULL);

    if (client_fd < 0)
        return;

    setNonBlocking(client_fd);

    clients[client_fd] = Connection(client_fd);

    std::cout << "New client connected\n";

    struct kevent client_event;

    EV_SET(&client_event,
           client_fd,
           EVFILT_READ,
           EV_ADD,
           0,
           0,
           NULL);

    kevent(kq,
           &client_event,
           1,
           NULL,
           0,
           NULL);
}

// =====================================================
// HANDLE READ EVENT
// =====================================================

void Server::handleReadEvent(int fd)
{
    char temp[1024];

    int bytes = recv(fd,
                     temp,
                     sizeof(temp),
                     0);

    // ---------- Disconnect ----------
    if (bytes <= 0)
    {
        disconnectClient(fd);
        return;
    }

    // ---------- Append Incoming Data ----------
    clients[fd].inputBuffer.append(temp, bytes);

    // ---------- Extract Complete Messages ----------
    auto messages =
        Protocol::extractMessages(
            clients[fd].inputBuffer);

    for (const auto &message : messages)
    {

        if (message.rfind("/name ", 0) == 0)
        {

            std::string new_name =
                message.substr(6);

            clients[fd].username = new_name;

            std::cout << "Client renamed to "
                      << new_name
                      << "\n";

            continue;
        }
        std::cout << "Message: "
                  << message
                  << "\n";
        std::string formatted =
            "[" + clients[fd].username + "]: " + message;

        broadcastMessage(fd, formatted);
    }
}

// =====================================================
// HANDLE WRITE EVENT
// =====================================================

void Server::handleWriteEvent(int fd)
{
    Connection &conn = clients[fd];

    if (!conn.outputBuffer.empty())
    {

        int sent = send(fd,
                        conn.outputBuffer.data(),
                        conn.outputBuffer.size(),
                        0);

        if (sent > 0)
        {
            conn.outputBuffer.erase(0, sent);
        }
    }

    // ---------- Fully Sent ----------
    if (conn.outputBuffer.empty())
    {

        struct kevent disable_event;

        EV_SET(&disable_event,
               fd,
               EVFILT_WRITE,
               EV_DELETE,
               0,
               0,
               NULL);

        kevent(kq,
               &disable_event,
               1,
               NULL,
               0,
               NULL);
    }
}

// =====================================================
// BROADCAST MESSAGE
// =====================================================

void Server::broadcastMessage(int sender_fd,
                              const std::string &message)
{
    for (auto &[client_fd, connection] : clients)
    {

        if (client_fd == sender_fd)
            continue;

        // ---------- Serialize Message ----------
        Protocol::appendMessage(
            connection.outputBuffer,
            message);

        // ---------- Enable WRITE Event ----------
        struct kevent write_event;

        EV_SET(&write_event,
               client_fd,
               EVFILT_WRITE,
               EV_ADD,
               0,
               0,
               NULL);

        kevent(kq,
               &write_event,
               1,
               NULL,
               0,
               NULL);
    }
}

// =====================================================
// DISCONNECT CLIENT
// =====================================================

void Server::disconnectClient(int fd)
{
    std::cout << "Client disconnected\n";

    close(fd);

    clients.erase(fd);

    struct kevent del_event;

    EV_SET(&del_event,
           fd,
           EVFILT_READ,
           EV_DELETE,
           0,
           0,
           NULL);

    kevent(kq,
           &del_event,
           1,
           NULL,
           0,
           NULL);
}