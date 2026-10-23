//
// Created by Terry Zha on 10/20/26.
//

#ifndef TRDP_CAPTURER_H
#define TRDP_CAPTURER_H

#include <queue>
#include <thread>

#include "common/Channel.hpp"

namespace capture {

using FrameChannel = syncc::Channel<std::vector<char>>;

std::jthread runCapturer(std::string_view imagePath, const std::atomic<bool>& shouldContinue, FrameChannel::Tx tx);

}

#endif //TRDP_CAPTURER_H
