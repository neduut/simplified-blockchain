#pragma once

#include <chrono>
#include <string>

class Timer {
public:
    Timer() { reset(); }

    void reset() {
        start = std::chrono::high_resolution_clock::now();
    }

    //milisek
    double elapsed_ms() const {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> diff = end - start;
        return diff.count();
    }

    //sekundės
    double elapsed() const {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        return diff.count();
    }

    //mikrosek
    double elapsed_us() const {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> diff = end - start;
        return diff.count();
    }

private:
    std::chrono::high_resolution_clock::time_point start;
};
