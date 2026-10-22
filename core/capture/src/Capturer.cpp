//
// Created by Terry Zha on 10/20/26.
//

#include "capture/Capturer.h"

#include <chrono>
#include <fstream>

namespace capture {
std::jthread runCapturer(std::string_view imagePath, const std::atomic<bool>& shouldContinue, FrameChannel::Tx tx) {
    return std::jthread([imagePath, &shouldContinue, tx = std::move(tx)] {
        using clock = std::chrono::steady_clock;
        constexpr auto interval = std::chrono::duration_cast<clock::duration>(std::chrono::duration<double>(1.0/30.0));

        std::ifstream file(imagePath.data(),std::ios::in | std::ios::binary);
        if (!file) {
            throw std::runtime_error("Failed to open image file");
        }
        std::vector<char> imageBytes(
            (std::istreambuf_iterator<char>(file)),
            std::istreambuf_iterator<char>()
        );

        auto next = clock::now();
        while (shouldContinue) {
            next += interval;
            tx->send(imageBytes);
            auto now = std::chrono::steady_clock::now();
            if (now < next) {
                std::this_thread::sleep_until(next);
            } else {
                next = now;
            }
        }
    });
}
}
