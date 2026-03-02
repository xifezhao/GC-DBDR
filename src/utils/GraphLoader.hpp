#pragma once

#include "../core/Types.hpp"

#include <string>
#include <vector>
#include <fstream>
#include <sstream>
#include <unordered_map>
#include <iostream>
#include <regex>
#include <algorithm>

/**
 * @brief Utility class to load graphs from file systems.
 */
class GraphLoader {
public:
    GraphLoader() = default;

    std::vector<ConcreteEdge> load_edge_list(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open file " << filepath << std::endl;
            exit(EXIT_FAILURE);
        }

        std::vector<ConcreteEdge> edges;
        std::string line, u_str, v_str;
        char label;

        while (std::getline(file, line)) {
            if (line.empty() || line[0] == '#') continue; 
            std::stringstream ss(line);
            if (ss >> u_str >> v_str >> label) {
                NodeID u = get_or_create_id(u_str);
                NodeID v = get_or_create_id(v_str);
                edges.push_back({u, v, label});
            }
        }

        std::cout << "[GraphLoader] Loaded " << edges.size() << " edges, " 
                  << node_counter << " nodes from " << filepath << "\n";
        return edges;
    }

    std::vector<ConcreteEdge> load_dot(const std::string& filepath) {
        std::ifstream file(filepath);
        if (!file.is_open()) {
            std::cerr << "Error: Could not open DOT file " << filepath << std::endl;
            exit(EXIT_FAILURE);
        }

        std::vector<ConcreteEdge> edges;
        std::string line;
        
        // FIX: 使用自定义定界符 "rgx" 避免 Raw String 解析错误
        std::regex edge_pat(R"rgx(\s*"?([a-zA-Z0-9_]+)"?\s*->\s*"?([a-zA-Z0-9_]+)"?\s*\[.*label="?([a-zA-Z0-9\(\)])"?.*\];)rgx");
        std::smatch matches;

        while (std::getline(file, line)) {
            if (std::regex_search(line, matches, edge_pat)) {
                if (matches.size() == 4) {
                    std::string u_str = matches[1].str();
                    std::string v_str = matches[2].str();
                    std::string l_str = matches[3].str();

                    NodeID u = get_or_create_id(u_str);
                    NodeID v = get_or_create_id(v_str);
                    Label label = l_str.empty() ? 'e' : l_str[0]; 

                    edges.push_back({u, v, label});
                }
            }
        }

        std::cout << "[GraphLoader] Loaded " << edges.size() << " edges (DOT), " 
                  << node_counter << " nodes from " << filepath << "\n";
        return edges;
    }

    std::string get_name(NodeID id) const {
        if (id < id_to_name.size()) return id_to_name[id];
        return "Unknown";
    }

    size_t num_nodes() const {
        return node_counter;
    }

private:
    std::unordered_map<std::string, NodeID> name_to_id;
    std::vector<std::string> id_to_name;
    NodeID node_counter = 0;

    NodeID get_or_create_id(const std::string& name) {
        if (name_to_id.find(name) == name_to_id.end()) {
            name_to_id[name] = node_counter;
            id_to_name.push_back(name);
            node_counter++;
        }
        return name_to_id[name];
    }
};