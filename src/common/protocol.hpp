#pragma once

#include <string>
#include <vector>

class Protocol {
public:

    // serialize message into output buffer
    static void appendMessage(
        std::string& outputBuffer,
        const std::string& message
    );

    // extract complete messages from input buffer
    static std::vector<std::string>
    extractMessages(std::string& inputBuffer);
};