/**
 * GC-DBDR: Correct-by-Construction Dynamic Reachability
 * Main Demo Entry Point
 * 
 * This file demonstrates the basic usage of the GC-DBDR solver, illustrating:
 * 1. Incremental updates (Edge Insertion) triggering reachability discovery.
 * 2. Decremental updates (Edge Deletion) correctly retracting facts.
 * 3. The "Alternative Path" scenario verifying the Counting-Augmented Domain.
 */

#include <iostream>
#include <string>
#include <cassert>
#include <vector>

// 包含核心算法头文件
#include "core/GC_DBDR.hpp"

// 辅助函数：打印测试结果
void assert_reachability(GC_DBDR& solver, NodeID u, NodeID v, bool expected, const std::string& desc) {
    bool result = solver.query(u, v);
    std::cout << "[" << (result == expected ? "PASS" : "FAIL") << "] " 
              << desc << " | Query(" << u << " -> " << v << "): " 
              << (result ? "Reachable" : "Unreachable") 
              << " (Expected: " << (expected ? "True" : "False") << ")" << std::endl;
    
    if (result != expected) {
        std::cerr << "!!! Assertion Failed: Logic Error Detected !!!" << std::endl;
        std::exit(EXIT_FAILURE);
    }
}

int main() {
    std::cout << "========================================================\n";
    std::cout << "   GC-DBDR: Dynamic Bidirected Dyck Reachability Demo   \n";
    std::cout << "   (c) FM 2026 Artifact Evaluation                      \n";
    std::cout << "========================================================\n\n";

    // 1. 初始化求解器
    GC_DBDR solver;
    std::cout << "-> Solver initialized.\n\n";

    // 定义节点和标签
    // 场景：模拟简单的函数调用匹配 S -> (_i S )_i
    // Labels: 'a' = Open Bracket '(', 'b' = Close Bracket ')'
    NodeID n1 = 1, n2 = 2, n3 = 3, n4 = 4;
    Label open_tag = 'a'; 
    Label close_tag = 'b'; 

    // =========================================================
    // Scenario 1: Basic Reachability (Incremental)
    // Path: 1 --a--> 2 --b--> 3
    // Result: 1 reaches 3 via matched Dyck path "ab"
    // =========================================================
    std::cout << "--- Scenario 1: Incremental Insertion ---\n";
    
    std::cout << "Action: Insert 1 --a--> 2\n";
    solver.insert_edge(n1, n2, open_tag);
    
    assert_reachability(solver, n1, n3, false, "Intermediate state (incomplete path)");

    std::cout << "Action: Insert 2 --b--> 3\n";
    solver.insert_edge(n2, n3, close_tag);

    // 验证：1 -> 3 应该是可达的
    assert_reachability(solver, n1, n3, true,  "Complete Dyck path (1->2->3)");


    // =========================================================
    // Scenario 2: Alternative Paths (The Ghost Path Problem)
    // Current: 1 -> 3 (via Node 2)
    // Action: Add another path 1 --a--> 4 --b--> 3
    // Result: 1 -> 3 is supported by TWO derivations.
    // =========================================================
    std::cout << "\n--- Scenario 2: Adding Alternative Path ---\n";
    
    std::cout << "Action: Insert 1 --a--> 4\n";
    solver.insert_edge(n1, n4, open_tag);
    
    std::cout << "Action: Insert 4 --b--> 3\n";
    solver.insert_edge(n4, n3, close_tag);

    assert_reachability(solver, n1, n3, true,  "Reachability maintained (Double support)");


    // =========================================================
    // Scenario 3: Decremental Update (Safety Check)
    // Action: Delete the original path (via Node 2)
    // Result: 1 -> 3 MUST still be reachable via Node 4.
    //         If the algorithm was naive, it might delete 1->3.
    // =========================================================
    std::cout << "\n--- Scenario 3: Decremental Update (Preserving Alternatives) ---\n";

    std::cout << "Action: Delete 1 --a--> 2\n";
    solver.delete_edge(n1, n2, open_tag);

    // 检查：1->3 是否依然可达？(由 1->4->3 支撑)
    assert_reachability(solver, n1, n3, true,  "Safety: Alternative path preserves reachability");


    // =========================================================
    // Scenario 4: Full Erasure (Precision Check)
    // Action: Delete the remaining path (via Node 4)
    // Result: 1 -> 3 MUST become unreachable.
    // =========================================================
    std::cout << "\n--- Scenario 4: Decremental Update (Full Erasure) ---\n";

    std::cout << "Action: Delete 4 --b--> 3\n";
    solver.delete_edge(n4, n3, close_tag);

    // 检查：1->3 应该不可达
    assert_reachability(solver, n1, n3, false, "Precision: Path retracted after support reaches zero");

    
    // =========================================================
    // Final Stats
    // =========================================================
    std::cout << "\n========================================================\n";
    std::cout << "   Memory Stats:\n";
    std::cout << "   - Hypergraph Size: " << solver.get_hypergraph_size() << " elements\n";
    std::cout << "========================================================\n";
    std::cout << "   DEMO PASSED SUCCESSFULLY.\n";

    return 0;
}