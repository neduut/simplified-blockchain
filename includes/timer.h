#pragma once

#include <chrono>
#include <string>

// RAII Timer klasė: automatinis laiko matavimas
class Timer {
public:
    Timer() noexcept { reset(); }
    
    // isjungia kopijavima (timer yra unique per scope)
    Timer(const Timer&) = delete;
    Timer& operator=(const Timer&) = delete;
    
    // move 
    Timer(Timer&&) noexcept = default;
    Timer& operator=(Timer&&) noexcept = default;
    
    ~Timer() = default;

    void reset() noexcept {
        start = std::chrono::high_resolution_clock::now();
    }

    // milisekundes
    double elapsed_ms() const noexcept {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::milli> diff = end - start;
        return diff.count();
    }

    // sekundes
    double elapsed() const noexcept {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double> diff = end - start;
        return diff.count();
    }

    // mikrosekundes
    double elapsed_us() const noexcept {
        auto end = std::chrono::high_resolution_clock::now();
        std::chrono::duration<double, std::micro> diff = end - start;
        return diff.count();
    }

private:
    std::chrono::high_resolution_clock::time_point start;
};
