/**
 * @file Exp_Efficiency.cpp
 * @brief RQ1 Experiment: Efficiency Comparison (Incremental vs. Batch)
 * 
 * This experiment executes the "Controlled Mutation Protocol" described in 
 * Section 7.1 of the paper. It compares the execution time of GC-DBDR 
 * against the Fast-CFL baseline.
 * 
 * Output: data/results/efficiency_log.csv
 */

#include "../core/GC_DBDR.hpp"
#include "../core/Fast_CFL.hpp"
#include "../utils/PerfTimer.hpp"
#include "../utils/GraphLoader.hpp"
#include "../utils/MemoryTracker.hpp"

#include <iostream>
#include <fstream>
#include <vector>
#include <random>
#include <algorithm>
#include <iomanip>

// =========================================================
// Experiment Configuration
// =========================================================
const int DEFAULT_NUM_NODES = 500;       // N for synthetic graph
const int DEFAULT_NUM_MUTATIONS = 1000;  // Number of operations
const int INITIAL_EDGES = 1000;          // Initial density
const std::string OUTPUT_CSV = "data/results/efficiency_log.csv";

// Helper to keep track of current active edges in the graph
// (Needed to pick valid edges for deletion)
struct EdgeInfo {
    NodeID u, v;
    Label label;
    bool operator==(const EdgeInfo& o) const {
        return u == o.u && v == o.v && label == o.label;
    }
};

int main(int argc, char** argv) {
    std::cout << "========================================================\n";
    std::cout << "   RQ1: Efficiency Experiment (GC-DBDR vs Batch)        \n";
    std::cout << "   (c) FM 2026 Artifact Evaluation                      \n";
    std::cout << "========================================================\n";

    // 1. Setup Generators
    int num_nodes = DEFAULT_NUM_NODES;
    int num_mutations = DEFAULT_NUM_MUTATIONS;
    
    // Use fixed seed for reproducibility
    std::mt19937 rng(42); 
    std::uniform_int_distribution<NodeID> node_dist(0, num_nodes - 1);
    // Labels: 'a'/'b' as parentheses, 'e' as epsilon/neutral
    std::vector<Label> labels = {'a', 'b', 'a', 'b', 'e'}; 
    std::uniform_int_distribution<int> label_dist(0, labels.size() - 1);

    // 2. Initialize Solvers
    GC_DBDR inc_solver;
    Fast_CFL batch_solver; // Note: Batch solver is stateless between runs in this exp logic
    
    // Active edges database (to simulate the "Program Graph" state)
    std::vector<EdgeInfo> active_edges;

    // 3. Open Log File
    std::ofstream csv(OUTPUT_CSV);
    if (!csv.is_open()) {
        std::cerr << "Error: Cannot open " << OUTPUT_CSV << "\n";
        return 1;
    }
    // CSV Header matching plot_speedup.py expectations
    csv << "Operation,Time_GC_DBDR_us,Time_Batch_us,Delta_Size,Reachable_Count\n";

    std::cout << "-> Generating initial graph (" << num_nodes << " nodes, " << INITIAL_EDGES << " edges)...\n";

    // 4. Pre-warm: Generate Initial Graph
    for (int i = 0; i < INITIAL_EDGES; ++i) {
        NodeID u = node_dist(rng);
        NodeID v = node_dist(rng);
        Label l = labels[label_dist(rng)];
        
        active_edges.push_back({u, v, l});
        inc_solver.insert_edge(u, v, l);
    }
    std::cout << "-> Initial Graph Loaded. Hypergraph Size: " << inc_solver.get_hypergraph_size() << "\n\n";

    // 5. Mutation Loop
    std::cout << "-> Starting " << num_mutations << " mutations...\n";
    std::cout << "   Progress: [";

    long total_inc_time = 0;
    long total_batch_time = 0;

    for (int i = 0; i < num_mutations; ++i) {
        // Progress Bar
        if (i % (num_mutations / 20) == 0) { std::cout << "=" << std::flush; }

        // Decide Operation: 50% Insert, 50% Delete
        // But if graph is empty, must Insert. If too full, Delete.
        bool is_insert = (rng() % 2 == 0);
        if (active_edges.empty()) is_insert = true;

        long t_inc = 0;
        long t_batch = 0;
        size_t delta = 0;
        size_t reach_count = 0; // For sanity check

        if (is_insert) {
            // --- INSERTION ---
            NodeID u = node_dist(rng);
            NodeID v = node_dist(rng);
            Label l = labels[label_dist(rng)];
            
            // 1. Measure GC-DBDR
            {
                Timer t;
                inc_solver.insert_edge(u, v, l);
                t_inc = t.elapsed_microseconds();
            }
            
            // Update Ground Truth
            active_edges.push_back({u, v, l});

        } else {
            // --- DELETION ---
            // Pick a random existing edge
            std::uniform_int_distribution<size_t> edge_dist(0, active_edges.size() - 1);
            size_t idx = edge_dist(rng);
            EdgeInfo e = active_edges[idx];

            // Swap-and-pop for O(1) removal from vector
            active_edges[idx] = active_edges.back();
            active_edges.pop_back();

            // 1. Measure GC-DBDR
            {
                Timer t;
                inc_solver.delete_edge(e.u, e.v, e.label);
                t_inc = t.elapsed_microseconds();
            }
        }

        // Common: Measure Batch Baseline
        // Note: Ideally we run this every time, but for very large N, we might sample.
        // Here we run it every time for rigorous data.
        {
            // 2. Reconstruct Batch Solver State
            // (Simulate "Reading file from scratch")
            Fast_CFL fresh_batch;
            for (const auto& edge : active_edges) {
                fresh_batch.insert_edge(edge.u, edge.v, edge.label);
            }

            Timer t;
            fresh_batch.run_from_scratch();
            t_batch = t.elapsed_microseconds();
            
            reach_count = fresh_batch.get_reachable_count();
        }

        // Retrieve Delta size from solver (MIR size)
        delta = inc_solver.get_last_delta_size();
        
        // Log to CSV
        csv << (is_insert ? "INS" : "DEL") << "," 
            << t_inc << "," 
            << t_batch << "," 
            << delta << ","
            << reach_count << "\n";

        total_inc_time += t_inc;
        total_batch_time += t_batch;
    }

    std::cout << "] Done.\n\n";

    // 6. Summary
    std::cout << "--------------------------------------------------------\n";
    std::cout << "Results Summary:\n";
    std::cout << "Total Mutations: " << num_mutations << "\n";
    std::cout << "Total Time (GC-DBDR): " << total_inc_time / 1000.0 << " ms\n";
    std::cout << "Total Time (Batch):   " << total_batch_time / 1000.0 << " ms\n";
    
    double speedup = (double)total_batch_time / (double)total_inc_time;
    std::cout << "Avg Speedup Factor:   " << std::fixed << std::setprecision(2) << speedup << "x\n";
    std::cout << "Data saved to: " << OUTPUT_CSV << "\n";
    std::cout << "--------------------------------------------------------\n";

    csv.close();
    return 0;
}