/**
 * @file Exp_Memory.cpp
 * @brief RQ2 Experiment: Memory Scalability Analysis (Enhanced UX)
 */

#include "../core/GC_DBDR.hpp"
#include "../core/Fast_CFL.hpp"
#include "../utils/MemoryTracker.hpp"
#include "../utils/PerfTimer.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <iomanip>
#include <chrono>

// =========================================================
// Experiment Configuration
// =========================================================
const int NUM_NODES = 1000;         
const int MAX_EDGES = 10000;        
const int STEP_SIZE = 500;          // 每 500 条边记录一次
const std::string OUTPUT_CSV = "data/results/memory_log.csv";

struct DataPoint {
    int num_edges;
    size_t rss_gc_dbdr; 
    size_t rss_batch;   
};

// 辅助函数：格式化时间
std::string format_duration(double seconds) {
    if (seconds < 60) {
        return std::to_string((int)seconds) + "s";
    } else {
        int min = (int)seconds / 60;
        int sec = (int)seconds % 60;
        return std::to_string(min) + "m " + std::to_string(sec) + "s";
    }
}

int main() {
    std::cout << "========================================================\n";
    std::cout << "   RQ2: Memory Scalability Experiment (Verbose Mode)    \n";
    std::cout << "   (c) FM 2026 Artifact Evaluation                      \n";
    std::cout << "========================================================\n";

    std::vector<DataPoint> results;
    
    std::mt19937 rng(12345);
    std::uniform_int_distribution<NodeID> node_dist(0, NUM_NODES - 1);
    std::vector<Label> labels = {'a', 'b', 'a', 'b', 'e'};
    std::uniform_int_distribution<int> label_dist(0, labels.size() - 1);

    std::cout << "-> Pre-generating mutation sequence (" << MAX_EDGES << " edges)...\n";
    std::vector<ConcreteEdge> mutation_log;
    mutation_log.reserve(MAX_EDGES);
    for (int i = 0; i < MAX_EDGES; ++i) {
        mutation_log.push_back({
            node_dist(rng), 
            node_dist(rng), 
            labels[label_dist(rng)]
        });
    }

    size_t base_rss = MemoryTracker::get_current_rss();
    std::cout << "-> Baseline RSS: " << MemoryTracker::to_mb(base_rss) << " MB\n\n";

    // ---------------------------------------------------------
    // Phase 1: GC-DBDR Growth
    // ---------------------------------------------------------
    std::cout << "--- Phase 1: Measuring GC-DBDR Memory Growth ---\n";
    {
        GC_DBDR inc_solver;
        auto start_time = std::chrono::steady_clock::now();
        
        for (int i = 0; i < MAX_EDGES; ++i) {
            const auto& edge = mutation_log[i];
            inc_solver.insert_edge(edge.u, edge.v, edge.label);

            if ((i + 1) % STEP_SIZE == 0) {
                size_t current_rss = MemoryTracker::get_current_rss();
                results.push_back({ i + 1, current_rss, 0 });

                // 简单的进度显示
                int percent = (i + 1) * 100 / MAX_EDGES;
                std::cout << "\r   [GC-DBDR] Progress: " << percent << "% (" 
                          << (i + 1) << "/" << MAX_EDGES << " edges) | RSS: " 
                          << std::fixed << std::setprecision(1) << MemoryTracker::to_mb(current_rss) << " MB" 
                          << std::flush;
            }
        }
        auto end_time = std::chrono::steady_clock::now();
        std::chrono::duration<double> diff = end_time - start_time;
        std::cout << "\n   -> Phase 1 finished in " << format_duration(diff.count()) << ".\n\n";
    } 

    // ---------------------------------------------------------
    // Phase 2: Batch Baseline Comparison (The Slow Part)
    // ---------------------------------------------------------
    std::cout << "--- Phase 2: Measuring Batch Baseline Memory ---\n";
    std::cout << "   Warning: This phase runs O(N^3) batch analysis repeatedly.\n";
    std::cout << "   It may take a long time. Please be patient.\n\n";
    
    int total_steps = results.size();
    auto p2_start_time = std::chrono::steady_clock::now();

    for (int idx = 0; idx < total_steps; ++idx) {
        auto step_start = std::chrono::steady_clock::now();
        
        DataPoint& point = results[idx];
        int edges_target = point.num_edges;

        // 1. Build
        Fast_CFL batch_solver;
        for (int i = 0; i < edges_target; ++i) {
            const auto& edge = mutation_log[i];
            batch_solver.insert_edge(edge.u, edge.v, edge.label);
        }

        // 2. Run (Heavy computation)
        batch_solver.run_from_scratch();

        // 3. Measure
        point.rss_batch = MemoryTracker::get_current_rss();

        auto step_end = std::chrono::steady_clock::now();
        std::chrono::duration<double> step_dur = step_end - step_start;
        std::chrono::duration<double> total_elapsed = step_end - p2_start_time;

        // Calculate ETA
        double avg_time_per_step = total_elapsed.count() / (idx + 1);
        double remaining_seconds = avg_time_per_step * (total_steps - 1 - idx);

        // 使用换行打印，防止长时间卡住不动让用户以为死机
        std::cout << "   [Step " << (idx + 1) << "/" << total_steps << "] "
                  << "Edges: " << std::setw(5) << edges_target 
                  << " | Batch RSS: " << std::setw(7) << std::fixed << std::setprecision(1) << MemoryTracker::to_mb(point.rss_batch) << " MB"
                  << " | Took: " << std::setw(5) << std::setprecision(1) << step_dur.count() << "s"
                  << " | ETA: " << format_duration(remaining_seconds)
                  << "\n";
    }
    std::cout << "\n-> Phase 2 Complete.\n\n";

    // ---------------------------------------------------------
    // Output to CSV
    // ---------------------------------------------------------
    std::ofstream csv(OUTPUT_CSV);
    if (csv.is_open()) {
        csv << "Edges,GC_DBDR_MB,Batch_MB,Growth_Factor\n";
        for (const auto& p : results) {
            double gc_mb = MemoryTracker::to_mb(p.rss_gc_dbdr);
            double batch_mb = MemoryTracker::to_mb(p.rss_batch);
            double factor = (batch_mb > 0) ? (gc_mb / batch_mb) : 0.0;

            csv << p.num_edges << "," 
                << std::fixed << std::setprecision(3) << gc_mb << "," 
                << batch_mb << ","
                << factor << "\n";
        }
        std::cout << "-> Results saved to " << OUTPUT_CSV << "\n";
    } else {
        std::cerr << "Error: Could not write to CSV.\n";
    }

    return 0;
}