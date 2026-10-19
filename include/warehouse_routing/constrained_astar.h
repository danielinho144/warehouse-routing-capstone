#pragma once
#include <vector>
#include "warehouse_routing/graph.h"
#include "warehouse_routing/constraint.h"

namespace warehouse_routing {
    std::vector<int> constrainedAStar(const Graph& graph, int agentID, int startID, int goalID, const std::vector<constraintInfo>& constraints, int maxTick);
}