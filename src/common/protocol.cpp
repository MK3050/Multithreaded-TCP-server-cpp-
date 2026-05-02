#include "protocol.hpp"

#include <arpa/inet.h>
#include <cstring>

void Protocol::appendMessage(
    std::string& outputBuffer,
    const std::string& message)
{
    uint32_t len = message.size();
    uint32_t net_len = htonl(len);

    outputBuffer.append(
        (char*)&net_len,
        sizeof(net_len)
    );

    outputBuffer.append(message);
}

std::vector<std::string>
Protocol::extractMessages(std::string& inputBuffer)
{
    std::vector<std::string> messages;

    while (true) {

        // Need 4 bytes for length
        if (inputBuffer.size() < 4)
            break;

        uint32_t net_len;

        std::memcpy(&net_len,
                    inputBuffer.data(),
                    4);

        uint32_t len = ntohl(net_len);

        // Full message not available
        if (inputBuffer.size() < 4 + len)
            break;

        // Extract message
        std::string message =
            inputBuffer.substr(4, len);

        messages.push_back(message);

        // Remove processed message
        inputBuffer.erase(0, 4 + len);
    }

    return messages;
}