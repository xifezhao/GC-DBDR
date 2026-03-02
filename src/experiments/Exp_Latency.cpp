/**
 * @file Exp_Latency.cpp
 * @brief RQ3 Experiment: Responsiveness and Tail Latency Analysis
 */

#include "../core/GC_DBDR.hpp"
#include "../utils/PerfTimer.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>
#include <numeric>
#include <iomanip>

const int NUM_NODES = 2000;         
const int INITIAL_EDGES = 5000;     
const int NUM_MUTATIONS = 50000;    
const std::string OUTPUT_CSV = "data/results/latency_log.csv";

struct ShadowEdge {
    NodeID u, v;
    Label label;
};

int main() {
    std::cout << "========================================================\n";
    std::cout << "   RQ3: Latency Distribution Experiment (IDE Simulation)\n";
    std::cout << "   (c) FM 2026 Artifact Evaluation                      \n";
    std::cout << "========================================================\n";

    GC_DBDR solver;
    std::vector<ShadowEdge> active_edges;
    active_edges.reserve(INITIAL_EDGES + NUM_MUTATIONS);

    std::mt19937 rng(999); 
    std::uniform_int_distribution<NodeID> node_dist(0, NUM_NODES - 1);
    std::vector<Label> labels = {'a', 'b', 'a', 'b', 'e'}; 
    std::uniform_int_distribution<int> label_dist(0, labels.size() - 1);

    std::cout << "-> Pre-warming graph with " << INITIAL_EDGES << " edges...\n";
    for (int i = 0; i < INITIAL_EDGES; ++i) {
        NodeID u = node_dist(rng);
        NodeID v = node_dist(rng);
        Label l = labels[label_dist(rng)];
        
        solver.insert_edge(u, v, l);
        active_edges.push_back({u, v, l});
    }
    std::cout << "-> Initial Hypergraph Size: " << solver.get_hypergraph_size() << "\n\n";

    std::cout << "-> Starting " << NUM_MUTATIONS << " atomic updates...\n";
    
    std::vector<long> latencies_us;
    latencies_us.reserve(NUM_MUTATIONS);

    // FIX: 正确的文件流构造方式
    std::ofstream csv(OUTPUT_CSV);
    if (!csv.is_open()) {
        std::cerr << "Error: Cannot open " << OUTPUT_CSV << "\n";
        return 1;
    }
    csv << "Latency_us,Op_Type,Delta_Size\n";

    for (int i = 0; i < NUM_MUTATIONS; ++i) {
        bool is_insert = (rng() % 2 == 0);
        if (active_edges.empty()) is_insert = true;

        long elapsed_us = 0;
        size_t delta_sz = 0;
        std::string op_type;

        if (is_insert) {
            NodeID u = node_dist(rng);
            NodeID v = node_dist(rng);
            Label l = labels[label_dist(rng)];
            
            Timer t;
            solver.insert_edge(u, v, l);
            elapsed_us = t.elapsed_microseconds();

            active_edges.push_back({u, v, l});
            op_type = "INS";
        } else {
            std::uniform_int_distribution<size_t> edge_idx_dist(0, active_edges.size() - 1);
            size_t idx = edge_idx_dist(rng);
            ShadowEdge e = active_edges[idx];

            active_edges[idx] = active_edges.back();
            active_edges.pop_back();

            Timer t;
            solver.delete_edge(e.u, e.v, e.label);
            elapsed_us = t.elapsed_microseconds();

            op_type = "DEL";
        }

        delta_sz = solver.get_last_delta_size();
        
        latencies_us.push_back(elapsed_us);
        csv << elapsed_us << "," << op_type << "," << delta_sz << "\n";
    }
    
    csv.close();

    std::sort(latencies_us.begin(), latencies_us.end());

    auto get_percentile = [&](double p) -> long {
        size_t idx = (size_t)(p * latencies_us.size());
        if (idx >= latencies_us.size()) idx = latencies_us.size() - 1;
        return latencies_us[idx];
    };

    long p50 = get_percentile(0.50);
    long p90 = get_percentile(0.90);
    long p95 = get_percentile(0.95);
    long p99 = get_percentile(0.99);
    long max_lat = latencies_us.back();
    
    double sum = std::accumulate(latencies_us.begin(), latencies_us.end(), 0.0);
    double avg = sum / latencies_us.size();

    std::cout << "\n--------------------------------------------------------\n";
    std::cout << "Results Analysis (" << NUM_MUTATIONS << " samples):\n";
    std::cout << "--------------------------------------------------------\n";
    std::cout << "Average Latency: " << std::fixed << std::setprecision(2) << avg << " us\n";
    std::cout << "Median  (P50):   " << p50 << " us\n";
    std::cout << "Tail    (P90):   " << p90 << " us\n";
    std::cout << "Tail    (P95):   " << p95 << " us\n";
    std::cout << "Tail    (P99):   " << p99 << " us\n";
    std::cout << "Maximum (Max):   " << max_lat << " us\n";
    std::cout << "--------------------------------------------------------\n";
    
    if (p99 < 100000) { 
        std::cout << "[SUCCESS] P99 is within interactive limits (< 100ms).\n";
    } else {
        std::cout << "[WARNING] P99 exceeds interactive limits.\n";
    }
    
    std::cout << "Raw data saved to: " << OUTPUT_CSV << "\n";

    return 0;
}