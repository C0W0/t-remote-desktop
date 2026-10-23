#ifndef TRDP_AARC_H
#define TRDP_AARC_H

#include <atomic>
#include <cstddef>
#include <memory>
#include <utility>

namespace syncc {
// Atomic access to a shared pointer, not to the object it points to.
// Uses the shared_ptr atomic free functions available through C++23.
template <typename T>
class AArc {
public:
    using value_type = std::shared_ptr<T>;
    static constexpr bool is_always_lock_free = false;

    AArc() noexcept = default;
    AArc(std::nullptr_t) noexcept {}
    AArc(value_type value) noexcept : value_(std::move(value)) {}

    AArc(const AArc&) = delete;
    AArc& operator=(const AArc&) = delete;

    value_type operator=(value_type desired) noexcept {
        store(desired);
        return desired;
    }

    operator value_type() const noexcept {
        return load();
    }

    bool is_lock_free() const noexcept {
        return std::atomic_is_lock_free(&value_);
    }

    value_type load(std::memory_order order = std::memory_order_seq_cst) const noexcept {
        return std::atomic_load_explicit(&value_, order);
    }

    void store(value_type desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        std::atomic_store_explicit(&value_, std::move(desired), order);
    }

    value_type exchange(value_type desired, std::memory_order order = std::memory_order_seq_cst) noexcept {
        return std::atomic_exchange_explicit(&value_, std::move(desired), order);
    }

    bool compare_exchange_strong(value_type& expected, value_type desired,
                                 std::memory_order success, std::memory_order failure) noexcept {
        return std::atomic_compare_exchange_strong_explicit(&value_, &expected, std::move(desired), success, failure);
    }

    bool compare_exchange_strong(value_type& expected, value_type desired,
                                 std::memory_order order = std::memory_order_seq_cst) noexcept {
        return compare_exchange_strong(expected, std::move(desired), order, failureOrder(order));
    }

    bool compare_exchange_weak(value_type& expected, value_type desired,
                               std::memory_order success, std::memory_order failure) noexcept {
        return std::atomic_compare_exchange_weak_explicit(&value_, &expected, std::move(desired), success, failure);
    }

    bool compare_exchange_weak(value_type& expected, value_type desired,
                               std::memory_order order = std::memory_order_seq_cst) noexcept {
        return compare_exchange_weak(expected, std::move(desired), order, failureOrder(order));
    }

private:
    static std::memory_order failureOrder(std::memory_order order) noexcept {
        if (order == std::memory_order_acq_rel) {
            return std::memory_order_acquire;
        }
        if (order == std::memory_order_release) {
            return std::memory_order_relaxed;
        }
        return order;
    }

    value_type value_;
};

} // namespace common

#endif // TRDP_AARC_H
