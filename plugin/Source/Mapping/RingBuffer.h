#pragma once

#include <array>
#include <atomic>
#include <optional>

namespace auralis::mapping
{
/**
    A simple single-producer/single-consumer ring buffer that avoids locks and
    heap allocations. Intended for lightweight message passing between the UI
    thread and the audio/MIDI thread.
*/
template <typename T, size_t Capacity>
class RingBuffer
{
public:
    RingBuffer() = default;

    bool push(const T& value) noexcept
    {
        auto write = writeIndex.load(std::memory_order_relaxed);
        auto next = increment(write);

        if (next == readIndex.load(std::memory_order_acquire))
            return false; // buffer full

        storage[write] = value;
        writeIndex.store(next, std::memory_order_release);
        return true;
    }

    [[nodiscard]] std::optional<T> pop() noexcept
    {
        T value {};
        if (! pop(value))
            return std::nullopt;

        return value;
    }

    bool pop(T& value) noexcept
    {
        auto read = readIndex.load(std::memory_order_relaxed);

        if (read == writeIndex.load(std::memory_order_acquire))
            return false; // buffer empty

        value = storage[read];
        readIndex.store(increment(read), std::memory_order_release);
        return true;
    }

    void reset() noexcept
    {
        readIndex.store(0, std::memory_order_release);
        writeIndex.store(0, std::memory_order_release);
    }

    [[nodiscard]] bool isEmpty() const noexcept
    {
        return readIndex.load(std::memory_order_acquire)
            == writeIndex.load(std::memory_order_acquire);
    }

    [[nodiscard]] bool isFull() const noexcept
    {
        auto next = increment(writeIndex.load(std::memory_order_acquire));
        return next == readIndex.load(std::memory_order_acquire);
    }

private:
    [[nodiscard]] constexpr size_t increment(size_t index) const noexcept
    {
        return (index + 1) % Capacity;
    }

    std::array<T, Capacity> storage {};
    std::atomic<size_t> writeIndex { 0 };
    std::atomic<size_t> readIndex { 0 };
};

} // namespace auralis::mapping
