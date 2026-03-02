#pragma once

#include <chrono>
#include <iostream>
#include <string>

/**
 * @brief High-precision timer wrapper for benchmarking.
 * 
 * Uses std::chrono::high_resolution_clock to measure execution time
 * with microsecond (us) or nanosecond (ns) granularity.
 * Essential for measuring the "fast path" of incremental updates in GC-DBDR.
 */
class Timer {
public:
    // Constructor starts the timer automatically
    Timer() {
        reset();
    }

    // Reset the starting point to "now"
    void reset() {
        m_start = std::chrono::high_resolution_clock::now();
    }

    /**
     * @brief Get elapsed time in microseconds (us).
     * This is the primary metric for RQ1 (Efficiency).
     */
    long long elapsed_microseconds() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::microseconds>(end - m_start).count();
    }

    /**
     * @brief Get elapsed time in nanoseconds (ns).
     * Useful if the update is extremely fast (e.g., O(1) no-op).
     */
    long long elapsed_nanoseconds() const {
        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::nanoseconds>(end - m_start).count();
    }

    /**
     * @brief Get elapsed time in milliseconds (ms).
     * Useful for batch analysis (Fast-CFL) or total runtime.
     */
    double elapsed_milliseconds() const {
        return elapsed_microseconds() / 1000.0;
    }

    /**
     * @brief Get elapsed time in seconds (s).
     */
    double elapsed_seconds() const {
        return elapsed_milliseconds() / 1000.0;
    }

private:
    std::chrono::high_resolution_clock::time_point m_start;
};

/**
 * @brief Helper class for RAII-style scope timing.
 * Prints the elapsed time to stdout upon destruction.
 * 
 * Usage:
 * {
 *     ScopedTimer t("Initialization");
 *     // ... heavy work ...
 * } // Prints "[Timer] Initialization: 1234 us" here
 */
class ScopedTimer : public Timer {
public:
    explicit ScopedTimer(const std::string& name) : m_name(name) {}

    ~ScopedTimer() {
        long long us = elapsed_microseconds();
        std::cout << "[Timer] " << m_name << ": " 
                  << us << " us (" << (us / 1000.0) << " ms)" << std::endl;
    }

private:
    std::string m_name;
};