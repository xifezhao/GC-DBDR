#pragma once

#include <iostream>
#include <string>
#include <iomanip>

// =========================================================
// Platform-specific includes
// =========================================================
#if defined(__linux__) || defined(__linux) || defined(__linux)
    #include <unistd.h>
    #include <sys/resource.h>
    #include <fstream>
#elif defined(__APPLE__) && defined(__MACH__)
    #include <unistd.h>
    #include <sys/resource.h>
    #include <mach/mach.h>
#elif defined(_WIN32)
    #include <windows.h>
    #include <psapi.h>
#else
    // Fallback for unknown systems
#endif

/**
 * @brief Memory usage tracker for RQ2 (Scalability) experiments.
 * 
 * It measures the Resident Set Size (RSS), which represents the 
 * actual physical memory used by the process (Derivation Hypergraph + Graph).
 */
class MemoryTracker {
public:
    /**
     * @brief Get the current physical memory usage (RSS) in bytes.
     * 
     * @return size_t Bytes used. Returns 0 if OS is not supported.
     */
    static size_t get_current_rss() {
#if defined(__linux__) || defined(__linux)
        // On Linux, read /proc/self/statm
        // Field 2 is RSS in pages
        long rss = 0;
        std::ifstream statm("/proc/self/statm");
        if (statm.is_open()) {
            std::string ignore;
            statm >> ignore >> rss;
        }
        statm.close();
        
        long page_size = sysconf(_SC_PAGESIZE);
        return rss * page_size;

#elif defined(__APPLE__) && defined(__MACH__)
        // On macOS, use mach kernel task_info
        struct mach_task_basic_info info;
        mach_msg_type_number_t infoCount = MACH_TASK_BASIC_INFO_COUNT;
        if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO,
                      (task_info_t)&info, &infoCount) != KERN_SUCCESS) {
            return 0;
        }
        return (size_t)info.resident_size;

#elif defined(_WIN32)
        // On Windows
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            return pmc.WorkingSetSize;
        }
        return 0;
#else
        return 0; // Unsupported platform
#endif
    }

    /**
     * @brief Get the peak memory usage (High Water Mark) during execution.
     */
    static size_t get_peak_rss() {
#if defined(_WIN32)
        PROCESS_MEMORY_COUNTERS_EX pmc;
        if (GetProcessMemoryInfo(GetCurrentProcess(), (PROCESS_MEMORY_COUNTERS*)&pmc, sizeof(pmc))) {
            return pmc.PeakWorkingSetSize;
        }
        return 0;
#else
        // POSIX systems (Linux/macOS) use getrusage
        struct rusage rusage;
        getrusage(RUSAGE_SELF, &rusage);
        
        #if defined(__APPLE__) && defined(__MACH__)
            // macOS returns bytes
            return (size_t)rusage.ru_maxrss;
        #else
            // Linux returns Kilobytes
            return (size_t)(rusage.ru_maxrss * 1024L);
        #endif
#endif
    }

    // =========================================================
    // Formatting Helpers
    // =========================================================

    static double to_mb(size_t bytes) {
        return bytes / (1024.0 * 1024.0);
    }

    static double to_gb(size_t bytes) {
        return bytes / (1024.0 * 1024.0 * 1024.0);
    }

    /**
     * @brief Print current memory stats to stdout.
     */
    static void print_usage(const std::string& label = "Memory") {
        size_t curr = get_current_rss();
        size_t peak = get_peak_rss();
        
        std::cout << "[" << label << "] "
                  << "Current: " << std::fixed << std::setprecision(2) << to_mb(curr) << " MB | "
                  << "Peak: "    << std::fixed << std::setprecision(2) << to_mb(peak) << " MB" 
                  << std::endl;
    }
};

/**
 * @brief Helper to measure memory delta of a specific operation.
 * 
 * Usage:
 * {
 *    MemoryDelta md;
 *    // ... allocate hypergraph ...
 *    size_t graph_size = md.get_delta();
 * }
 */
class MemoryDelta {
public:
    MemoryDelta() {
        start_bytes = MemoryTracker::get_current_rss();
    }

    size_t get_delta() const {
        size_t end_bytes = MemoryTracker::get_current_rss();
        if (end_bytes > start_bytes) {
            return end_bytes - start_bytes;
        }
        return 0; // Should not happen unless memory was freed elsewhere
    }

    void print_delta(const std::string& label) const {
        std::cout << "[" << label << "] Delta: " 
                  << MemoryTracker::to_mb(get_delta()) << " MB" << std::endl;
    }

private:
    size_t start_bytes;
};