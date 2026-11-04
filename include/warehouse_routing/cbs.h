#pragma once
#include <optional>
#include <unordered_map>
#include "warehouse_routing/graph.h"
#include "warehouse_routing/agent.h"
#include <vector>

namespace warehouse_routing {
    std::optional<std::unordered_map<int, std::vector<int>>> planCBSPaths(
        const Graph& graph, 
        const std::vector<Agent>& agents, const std::unordered_map<int, int>& goals,
        int maxTick, int maxExpandedNodes);
}