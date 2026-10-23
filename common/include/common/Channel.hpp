//
// Created by FNU Zhaluo on 10/21/26.
//

#ifndef TRDP_CHANNEL_H
#define TRDP_CHANNEL_H

#include <condition_variable>
#include <cstdint>
#include <optional>
#include <memory>
#include <tuple>
#include <mutex>
#include <utility>
#include <expected>
#include <queue>

namespace syncc {
template<typename T>
class ConcurrentQueue {
public:
    ConcurrentQueue() = default;

    std::optional<T> get() {
        std::unique_lock lock(mutex_);
        cv_.wait(lock, [this] { return closed_ || !queue_.empty(); });

        if (queue_.empty()) {
            return std::nullopt;
        }
        T result = std::move(queue_.front());
        queue_.pop();
        return result;
    }

    void push(const T& v) {
        {
            std::unique_lock lock(mutex_);
            queue_.push(v);
        }
        cv_.notify_one();
    }

    void push(T&& v) {
        {
            std::unique_lock lock(mutex_);
            queue_.push(std::move(v));
        }
        cv_.notify_one();
    }

    uint32_t size() const {
        return queue_.size();
    }

    bool empty() const {
        return queue_.empty();
    }

    ~ConcurrentQueue() {
        {
            std::unique_lock lock(mutex_);
            closed_ = true;
        }
        cv_.notify_all();
    }

private:
    std::condition_variable cv_{};
    std::mutex mutex_{};
    std::queue<T> queue_{};
    bool closed_{false};
};

enum class ChannelError: uint32_t {
    TxClosed,
    QueueClosed,
};

template<typename T>
class Channel : std::enable_shared_from_this<Channel<T> > {
public:
    using EventType = std::expected<T, ChannelError>;

    class Rx {
        friend class Channel<T>;

    public:
        EventType get() {
            std::optional<EventType> result = channel_->queue_.get();
            if (!result) {
                return std::unexpected{ChannelError::QueueClosed};
            }
            return *result;
        }

        Rx(const Rx&) = delete;
        Rx(Rx&& other) noexcept : channel_(std::move(other.channel_)) {};
        Rx& operator=(const Rx&) = delete;
        Rx& operator=(Rx&& other) noexcept {
            if (this == &other) {
                return *this;
            }
            this->channel_ = std::move(other.channel_);
            return *this;
        };

    private:
        explicit Rx(std::shared_ptr<Channel<T> > channel) : channel_(std::move(channel)) {}

        std::shared_ptr<Channel<T> > channel_;
    };

    class TxInner {
        friend class Channel<T>;

    public:
        void send(const T& v) {
            channel_->queue_.push(v);
        }

        void send(T&& v) {
            channel_->queue_.push(std::move(v));
        }

        TxInner(const TxInner&) = delete;
        TxInner(TxInner&& other) noexcept : channel_(std::move(other.channel_)) {};
        TxInner& operator=(const TxInner&) = delete;
        TxInner& operator=(TxInner&&) = default;

        ~TxInner() {
            if (channel_) {
                channel_->queue_.push(std::unexpected{ChannelError::TxClosed});
            }
        }

    private:
        explicit TxInner(std::shared_ptr<Channel<T> > channel) : channel_(std::move(channel)) {}

        std::shared_ptr<Channel<T> > channel_;
    };

    using Tx = std::shared_ptr<TxInner>;

    static std::tuple<Tx, Rx> createChannel() {
        std::shared_ptr<Channel<T>> channelPtr{new Channel<T>()};
        Tx tx{new TxInner(channelPtr)};
        Rx rx(channelPtr);
        return {std::move(tx), std::move(rx)};
    }

    Channel(const Channel&) = delete;
    Channel& operator=(const Channel&) = delete;

private:
    Channel() = default;

    ConcurrentQueue<EventType> queue_;
};
}

#endif //TRDP_CHANNEL_H
