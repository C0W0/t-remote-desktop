//
// Created by Terry Zha on 10/20/26.
//

#ifndef TRDP_CAPTURER_H
#define TRDP_CAPTURER_H

#include <queue>
#include <thread>

#include "common/RwLock.hpp"

namespace capture {
class FrameQueue {
public:
    FrameQueue(): queue_{std::queue<std::vector<char>>()} {}

    std::vector<char> get() {
        auto guard = queue_.lockMut();
        std::vector<char> res = std::move(guard->front());
        guard->pop();
        return res;
    }

    void push(const std::vector<char>& v) {
        auto guard = queue_.lockMut();
        guard->push(v);
    }

    void push(std::vector<char>&& v) {
        auto guard = queue_.lockMut();
        guard->push(std::move(v));
    }

    uint32_t size() const {
        return queue_->size();
    }
private:
    concurrency::ConcurrentQueue<std::vector<char>> queue_;
};

std::jthread runCapturer(std::string_view imagePath, const std::atomic<bool>& shouldContinue, FrameQueue& frameQueue);

}

#endif //TRDP_CAPTURER_H
