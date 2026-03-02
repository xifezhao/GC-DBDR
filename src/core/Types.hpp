#pragma once

#include <cstdint>
#include <functional>
#include <vector>
#include <iostream>

// =========================================================
// 1. Basic Primitive Types
// =========================================================

// 节点 ID (使用 32 位整数，足以支持千万级节点图)
using NodeID = uint32_t;

// 边标签 (例如: '(', ')', 'a', 'b')
// 在更复杂的实现中，这可以是整数 ID 映射到字符串表
using Label = char;

// =========================================================
// 2. Abstract Domain Types (Section 3.1)
// =========================================================

/**
 * @brief Abstract Fact: Represents a reachability summary (u, v).
 * In the abstract domain D#, this means "u reaches v via a valid Dyck path".
 */
struct Fact {
    NodeID u, v;

    // Equality operator required for unordered_map keys
    bool operator==(const Fact& other) const {
        return u == other.u && v == other.v;
    }

    // Less-than operator for sorting (useful for consistent output logs)
    bool operator<(const Fact& other) const {
        if (u != other.u) return u < other.u;
        return v < other.v;
    }
};

/**
 * @brief Custom Hash for Fact to be used in std::unordered_map/set.
 */
struct FactHash {
    std::size_t operator()(const Fact& f) const {
        // Simple bit-shift combine hash
        // In production, consider boost::hash_combine
        return std::hash<NodeID>{}(f.u) ^ (std::hash<NodeID>{}(f.v) << 1);
    }
};

// =========================================================
// 3. Concrete Graph Types (Section 2.1)
// =========================================================

/**
 * @brief Concrete Edge: A physical edge in the graph G = (V, E).
 * Corresponds to an input mutation.
 */
struct ConcreteEdge {
    NodeID u, v;
    Label label;

    bool operator==(const ConcreteEdge& o) const {
        return u == o.u && v == o.v && label == o.label;
    }
};

/**
 * @brief Hash for ConcreteEdge to act as a key in the Dependency Index.
 */
struct ConcreteEdgeHash {
    std::size_t operator()(const ConcreteEdge& k) const {
        std::size_t h1 = std::hash<NodeID>{}(k.u);
        std::size_t h2 = std::hash<NodeID>{}(k.v);
        std::size_t h3 = std::hash<Label>{}(k.label);
        return h1 ^ (h2 << 1) ^ (h3 << 2);
    }
};

// =========================================================
// 4. Derivation Hypergraph Types (Section 3.2)
// =========================================================

/**
 * @brief HyperEdge: Represents a logical derivation step.
 * 
 * A hyperedge captures the causal link: "Premises => Conclusion".
 * Storing this explicitly enables the "Correct-by-Construction" decremental update.
 */
struct HyperEdge {
    enum RuleType { 
        TRANSITIVITY,   // S -> S S
        MATCHING,       // S -> ( S )
        BASE_CASE       // S -> epsilon (or direct edge)
    };

    RuleType type;
    
    // The conclusion (Target Fact)
    Fact target; 

    // The premises (Antecedents)
    // A derivation can depend on existing Abstract Facts and/or Concrete Edges.
    std::vector<Fact> premise_facts;
    std::vector<ConcreteEdge> premise_edges;

    // Debug helper
    bool operator==(const HyperEdge& other) const {
        return target == other.target && 
               premise_facts == other.premise_facts &&
               premise_edges == other.premise_edges;
    }
};

// =========================================================
// 5. Utility / Debugging
// =========================================================

// Stream output for easier debugging
inline std::ostream& operator<<(std::ostream& os, const Fact& f) {
    return os << "(" << f.u << ", " << f.v << ")";
}

inline std::ostream& operator<<(std::ostream& os, const ConcreteEdge& e) {
    return os << e.u << " --[" << e.label << "]--> " << e.v;
}