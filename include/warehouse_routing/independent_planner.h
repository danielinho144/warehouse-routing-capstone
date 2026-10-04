#pragma once
#include <unordered_map>
#include "warehouse_routing/agent.h"
#include "warehouse_routing/graph.h"
#include <vector>

namespace warehouse_routing {
    std::unordered_map<int, std::vector<int>> planIndependentPaths(const Graph& graph, const std::vector<Agent>& agents, const std::unordered_map<int, int>& goals);
}