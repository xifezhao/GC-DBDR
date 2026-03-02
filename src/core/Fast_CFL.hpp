#pragma once

#include "Types.hpp"

#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <queue>
#include <iostream>

/**
 * @brief Fast-CFL: Standard Batch Algorithm for Dyck-CFL Reachability
 * 
 * This class implements the standard worklist-based algorithm (often O(N^3)) 
 * for computing All-Pairs Dyck-CFL reachability.
 * 
 * It serves as the BASELINE for evaluating the efficiency of GC-DBDR.
 * Unlike GC-DBDR, this solver does not support incremental updates;
 * it re-computes the entire reachability relation from scratch.
 */
class Fast_CFL {
public:
    Fast_CFL() = default;

    /**
     * @brief Build the graph structure.
     * Note: In the experiment, we call this iteratively, but for Fast-CFL,
     * the actual computation only happens when run_from_scratch() is called.
     */
    void insert_edge(NodeID u, NodeID v, Label label) {
        adj[u].push_back({v, label});
        rev_adj[v].push_back({u, label});
        nodes.insert(u);
        nodes.insert(v);
    }

    /**
     * @brief Run the batch analysis from scratch.
     * Clears all previous results and computes the Least Fixed Point (LFP).
     */
    void run_from_scratch() {
        // 1. Reset State
        reachable.clear();
        summary_adj.clear();
        summary_rev_adj.clear();
        
        // Worklist for chaotic iteration
        std::queue<Fact> worklist;

        // 2. Initialization (Base Cases)
        // Rule: S -> epsilon (Self-loops are always Dyck paths)
        // This seeds the worklist.
        for (NodeID n : nodes) {
            add_fact(n, n, worklist);
        }

        // 3. Fixpoint Iteration
        while (!worklist.empty()) {
            Fact curr = worklist.front();
            worklist.pop();

            NodeID u = curr.u;
            NodeID v = curr.v;

            // -------------------------------------------------
            // Rule 1: Transitivity (S -> S S)
            // -------------------------------------------------
            
            // Right Extension: (u, v) + (v, w) -> (u, w)
            // 查找所有以 v 为起点的已知 Fact
            if (summary_adj.count(v)) {
                for (NodeID w : summary_adj[v]) {
                    add_fact(u, w, worklist);
                }
            }

            // Left Extension: (k, u) + (u, v) -> (k, v)
            // 查找所有以 u 为终点的已知 Fact
            if (summary_rev_adj.count(u)) {
                for (NodeID k : summary_rev_adj[u]) {
                    add_fact(k, v, worklist);
                }
            }

            // -------------------------------------------------
            // Rule 2: Matching (S -> (_i S )_i)
            // Look for: p --(_i--> u  AND  v --)_i--> q
            // -------------------------------------------------
            
            // 遍历 u 的所有入边 (查找左括号)
            if (rev_adj.count(u)) {
                for (auto& edge_in : rev_adj[u]) {
                    NodeID p = edge_in.first;
                    Label l_open = edge_in.second;

                    if (!is_open_bracket(l_open)) continue;
                    Label l_close = get_inverse_label(l_open);

                    // 遍历 v 的所有出边 (查找匹配的右括号)
                    if (adj.count(v)) {
                        for (auto& edge_out : adj[v]) {
                            if (edge_out.second == l_close) {
                                NodeID q = edge_out.first;
                                add_fact(p, q, worklist);
                            }
                        }
                    }
                }
            }
        }
    }

    /**
     * @brief Query if a path exists.
     * Should only be called AFTER run_from_scratch().
     */
    bool query(NodeID u, NodeID v) const {
        return reachable.find({u, v}) != reachable.end();
    }

    // Returns the total number of reachable pairs found
    size_t get_reachable_count() const {
        return reachable.size();
    }

private:
    // =========================================================
    // Data Structures
    // =========================================================

    // Concrete Graph
    std::unordered_map<NodeID, std::vector<std::pair<NodeID, Label>>> adj;
    std::unordered_map<NodeID, std::vector<std::pair<NodeID, Label>>> rev_adj;
    std::unordered_set<NodeID> nodes;

    // Abstract Reachability State (Summary Edges)
    // Fact -> Exists?
    std::unordered_set<Fact, FactHash> reachable;
    
    // Indexing for O(1) extension lookup:
    // u -> [v1, v2...] means (u, v1), (u, v2) exist
    std::unordered_map<NodeID, std::vector<NodeID>> summary_adj;
    std::unordered_map<NodeID, std::vector<NodeID>> summary_rev_adj;

    // =========================================================
    // Helpers
    // =========================================================

    bool is_open_bracket(Label l) const { return l == 'a' || l == '('; }
    
    Label get_inverse_label(Label l) const {
        if (l == 'a') return 'b';
        if (l == 'b') return 'a';
        if (l == '(') return ')';
        if (l == ')') return '(';
        return 0;
    }

    // Helper to add a new fact and update indices
    void add_fact(NodeID u, NodeID v, std::queue<Fact>& wl) {
        Fact f{u, v};
        if (reachable.find(f) == reachable.end()) {
            reachable.insert(f);
            
            // Update Indices for Transitivity
            summary_adj[u].push_back(v);
            summary_rev_adj[v].push_back(u);
            
            wl.push(f);
        }
    }
};