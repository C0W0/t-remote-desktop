
#ifndef TRDP_RWLOCK_HPP
#define TRDP_RWLOCK_HPP
#include <shared_mutex>

namespace concurrency {
template <typename T>
class ReadLockProxy {
public:
    explicit ReadLockProxy(const T& data, std::shared_mutex& mutex): data_(data), lock_(mutex) {}
    const T* operator->() const { return &data_; }
private:
    const T& data_;
    std::shared_lock<std::shared_mutex> lock_;
};

template <typename T>
class WriteLockProxy {
public:
    explicit WriteLockProxy(T& data, std::shared_mutex& mutex): data_(data), lock_(mutex) {};
    T* operator->() { return &data_; }
private:
    T& data_;
    std::unique_lock<std::shared_mutex> lock_;
};

template <typename T>
class RwLock {
public:
    explicit RwLock(T&& data): data_(std::forward<T>(data)) {};

    ReadLockProxy<T> lock() const {
        return ReadLockProxy<T>{data_, mutex_};
    }

    WriteLockProxy<T> lockMut() {
        return WriteLockProxy<T>{data_, mutex_};
    }

    ReadLockProxy<T> operator->() const {
        return this->lock();
    }
private:
    T data_;
    mutable std::shared_mutex mutex_;
};

template <typename T>
using ConcurrentQueue = RwLock<std::queue<T>>;
}

#endif
