#pragma once

#include <chrono>

class Timer {
public:
    Timer()
        : started(std::chrono::steady_clock::now())
    { }

    uint64_t elapsedSeconds() const {
        return std::chrono::duration_cast<std::chrono::seconds>(std::chrono::steady_clock::now() - started).count();
    }

    uint64_t elapsedMilliSeconds() const {
        return std::chrono::duration_cast<std::chrono::milliseconds>(std::chrono::steady_clock::now() - started).count();
    }

    uint64_t elapsedUSeconds() const {
        return std::chrono::duration_cast<std::chrono::microseconds>(std::chrono::steady_clock::now() - started).count();
    }

private:
    std::chrono::time_point<std::chrono::steady_clock> started;
};