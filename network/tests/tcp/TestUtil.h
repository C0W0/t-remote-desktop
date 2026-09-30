#ifndef TRDP_NETWORK_TESTS_TESTUTIL_H
#define TRDP_NETWORK_TESTS_TESTUTIL_H

#include <atomic>
#include <chrono>
#include <cstdint>
#include <expected>
#include <future>
#include <memory>
#include <optional>
#include <random>
#include <span>
#include <string>
#include <string_view>
#include <thread>

#include "tcp/Socket.h"

namespace network::testutil {

using namespace std::chrono_literals;

// Generous: only reached when something hangs, in which case the test fails instead of blocking forever.
inline constexpr auto kTimeout = 5s;
inline constexpr const char* kLoopback = "127.0.0.1";

// Listen(port) has no way to report an OS-assigned port, so tests pick ports themselves.
// The base is random per process (ctest may run tests in parallel processes) and sits below the
// OS ephemeral ranges (32768+ on Linux, 49152+ on macOS/Windows) so it doesn't collide with outgoing
// connections. Each call returns a fresh port, which also avoids TIME_WAIT rebind failures, because
// the POSIX listener does not set SO_REUSEADDR.
inline uint16_t nextPort() {
    static std::atomic<uint16_t> port = [] {
        std::random_device rd;
        return static_cast<uint16_t>(20000 + rd() % 11000);
    }();
    return port++;
}

// Runs a callable on a detached thread and lets the test wait for its result with a timeout.
//
// If the callable blocks forever (the bug these tests exist to catch), waitFor() returns nullopt so the
// test can fail cleanly. The thread is detached, so the callable must only capture things that outlive
// the test body (shared_ptr copies, values), never references to locals.
template <typename R>
class AsyncResult {
public:
    template <typename F>
    explicit AsyncResult(F fn) {
        auto promise = std::make_shared<std::promise<R>>();
        future_ = promise->get_future();
        std::thread([promise, fn = std::move(fn)]() mutable { promise->set_value(fn()); }).detach();
    }

    [[nodiscard]] bool ready() const { return future_.wait_for(0ms) == std::future_status::ready; }

    std::optional<R> waitFor(std::chrono::milliseconds timeout = kTimeout) {
        if (future_.wait_for(timeout) != std::future_status::ready) {
            return std::nullopt;
        }
        return future_.get();
    }

private:
    std::future<R> future_;
};

inline std::shared_ptr<ListeningSocket> listenOn(uint16_t port) {
    auto result = ListeningSocket::Listen(port);
    if (!result.has_value()) {
        return nullptr;
    }
    return std::make_shared<ListeningSocket>(std::move(result).value());
}

inline std::expected<void, int> sendString(const ConnectionSocket& socket, std::string_view data) {
    return socket.send({data.data(), data.size()});
}

// Receives exactly `size` bytes (the buffer is exactly `size`, so the exact-size recv cannot over-read).
inline std::expected<std::string, int> recvExactly(const ConnectionSocket& socket, size_t size) {
    std::string buffer(size, '\0');
    auto result = socket.recv({buffer.data(), buffer.size()}, static_cast<int>(size));
    if (!result.has_value()) {
        return std::unexpected(result.error());
    }
    return buffer;
}

} // namespace network::testutil

#endif // TRDP_NETWORK_TESTS_TESTUTIL_H
