#pragma once
#include "warehouse_routing/graph.h"
#include "warehouse_routing/conflict.h"
#include <optional>

namespace warehouse_routing {
    bool hasVertexConflict(int nextNodeA, int nextNodeB);
    bool hasSwapConflict(int currentNodeA, int nextNodeA, int currentNodeB, int nextNodeB);
    bool hasDiagonalConflict(const Graph& graph, int currentNodeA, int nextNodeA, int currentNodeB, int nextNodeB);
    std::optional<Conflict> findFirstConflict(const Graph& graph, const std::unordered_map<int, std::vector<int>>& paths);
}