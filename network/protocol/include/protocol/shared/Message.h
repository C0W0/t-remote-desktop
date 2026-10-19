//
// Created by Terry Zha on 10/14/26.
//

#ifndef TRDP_MESSAGE_H
#define TRDP_MESSAGE_H

#include <string>
#include <vector>

namespace network {
// message packet format:
// | Header | message |
std::vector<char> serializeMessage(const std::string& message);
}

#endif //TRDP_MESSAGE_H
