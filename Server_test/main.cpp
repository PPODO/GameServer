#include <iostream>
#include "../Protocol/MessageProtocol.h"

int main() {
    auto iProcotol = util::MakeMessageProtocol(2, 1, 3);
    
    uint8_t message = 0, result = 0, reason = 0;
    util::ExtractMessageProtocol(iProcotol, message, result, reason);

    std::cout << (uint16_t)message << '\t' << (uint16_t)result << '\t' << (uint16_t)reason << std::endl;
    return 0;
}